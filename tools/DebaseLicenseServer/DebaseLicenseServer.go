package DebaseLicenseServer

import (
	"context"
	"crypto/ed25519"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"
	"regexp"
	"strings"
	"time"
	"unsafe"

	"heytoaster.com/DebaseLicenseServer/license"

	"cloud.google.com/go/firestore"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// #cgo CFLAGS: -DDebaseLicenseServer=1
// #include "Debase.h"
import "C"

const LicensesCollection = "Licenses"
const TrialsCollection = "Trials"
const TrialDuration = 7 * 24 * time.Hour
const MachineCountMax = 3

var db *firestore.Client

func init() {
	var err error
	db, err = firestore.NewClient(context.Background(), firestore.DetectProjectID)
	if err != nil {
		log.Fatalf("firestore.NewClient failed: %v", err)
	}
}

func hashStringSanitize(hash string) (string, error) {
	hash = strings.ToLower(hash)

	validHash := regexp.MustCompile(`^[a-f0-9]+$`)
	if !validHash.MatchString(hash) {
		return "", fmt.Errorf("invalid characters")
	}

	return hash, nil
}

func machineIdSanitize(mid license.MachineId) (license.MachineId, error) {
	if len(mid) != license.MachineIdLen {
		return "", fmt.Errorf("invalid length")
	}

	r, err := hashStringSanitize(string(mid))
	return license.MachineId(r), err
}

func userIdSanitize(uid license.UserId) (license.UserId, error) {
	if len(uid) != license.UserIdLen {
		return "", fmt.Errorf("invalid length")
	}

	r, err := hashStringSanitize(string(uid))
	return license.UserId(r), err
}

func licenseCodeSanitize(code license.LicenseCode) (license.LicenseCode, error) {
	str := strings.ToLower(string(code))
	regex := regexp.MustCompile(`^[a-z0-9]+$`)
	if !regex.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}
	return license.LicenseCode(str), nil
}

func reqSanitize(req license.HTTPRequest) (license.HTTPRequest, error) {
	var err error
	req.MachineId, err = machineIdSanitize(req.MachineId)
	if err != nil {
		return license.HTTPRequest{}, fmt.Errorf("invalid machine id (%v): %w", req.MachineId, err)
	}

	if req.UserId != "" {
		req.UserId, err = userIdSanitize(req.UserId)
		if err != nil {
			return license.HTTPRequest{}, fmt.Errorf("invalid user id (%v): %w", req.UserId, err)
		}
	}

	if req.LicenseCode != "" {
		req.LicenseCode, err = licenseCodeSanitize(req.LicenseCode)
		if err != nil {
			return license.HTTPRequest{}, fmt.Errorf("invalid license code (%v): %w", req.UserId, err)
		}
	}

	return req, nil
}

func trialLicenseCreate(mid license.MachineId) license.DBTrialLicense {
	expiration := time.Now().Add(TrialDuration)
	return license.DBTrialLicense{
		MachineId:  mid,
		Version:    uint32(C.DebaseVersion),
		Expiration: expiration.Unix(),
	}
}

func sealedLicense(lic license.HTTPLicense) (license.HTTPSealedLicense, error) {
	// Convert `lic` to json
	payload, err := json.Marshal(lic)
	if err != nil {
		return license.HTTPSealedLicense{}, fmt.Errorf("json.Marshal failed: %w", err)
	}

	seed := *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
	key := ed25519.NewKeyFromSeed(seed[:])
	sig := ed25519.Sign(key, payload)
	return license.HTTPSealedLicense{
		Payload:   string(payload),
		Signature: hex.EncodeToString(sig),
	}, nil
}

func handlerLicense(ctx context.Context, w http.ResponseWriter, req license.HTTPRequest) (license.HTTPResponse, error) {
	var licenseNotFoundErr = errors.New("No matching license was found.")
	var machineLimitErr = errors.New("The maximum number of machines has already been registered for this license code.")

	var respErr error
	var userLic license.DBUserLicense
	userLicsRef := db.Collection(LicensesCollection).Doc(string(req.UserId))
	err := db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Transactions can run multiple times, where the last one wins
		// So make sure that respErr is cleared by default, so it doesn't
		// contain a value from a previous transaction
		respErr = nil

		userLicsDoc, err := tx.Get(userLicsRef)
		if err != nil {
			if status.Code(err) == codes.NotFound {
				respErr = licenseNotFoundErr
			}
			return fmt.Errorf("tx.Get() failed for UserId=%v LicenseCode=%v: %w", req.UserId, req.LicenseCode, err)
		}

		var userLics license.DBUserLicenses
		err = userLicsDoc.DataTo(&userLics)
		if err != nil {
			return fmt.Errorf("userLicsDoc.DataTo failed: %w", err)
		}

		// Find a license that matches the given license code
		var match *license.DBUserLicense
		for _, userLic := range userLics.Licenses {
			if userLic.LicenseCode == req.LicenseCode {
				match = &userLic
				break
			}
		}

		// If we didn't find a license for the given license code, return an error
		if match == nil {
			respErr = licenseNotFoundErr
			return fmt.Errorf("no matching license for UserId=%v LicenseCode=%v", req.UserId, req.LicenseCode)
		}

		// We found a license for the given license code
		// If the given machine id already exists in the license, then we're done
		for _, mid := range match.MachineIds {
			if mid == req.MachineId {
				userLic = *match
				return nil
			}
		}

		// Otherwise, the machine id doesn't exist in the license, so we need to add it
		// Bail if we don't have any free machine id slots
		if len(match.MachineIds) >= MachineCountMax {
			respErr = machineLimitErr
			return fmt.Errorf("max machines already reached for UserId=%v LicenseCode=%v", req.UserId, req.LicenseCode)
		}

		userLic.MachineIds = append(userLic.MachineIds, req.MachineId)

		err = tx.Set(userLicsRef, userLic)
		if err != nil {
			return fmt.Errorf("tx.Set() failed for UserId=%v LicenseCode=%v", req.UserId, req.LicenseCode)
		}

		return nil
	})

	if err != nil {
		if respErr != nil {
			return license.HTTPResponse{Error: respErr.Error()}, nil
		} else {
			return license.HTTPResponse{}, fmt.Errorf("db.RunTransaction() failed: %w", err)
		}
	}

	sealed, err := sealedLicense(license.HTTPLicense{
		UserId:      req.UserId,
		LicenseCode: userLic.LicenseCode,
		MachineId:   req.MachineId,
		Version:     userLic.Version,
	})
	if err != nil {
		return license.HTTPResponse{}, fmt.Errorf("sealedLicense failed: %w", err)
	}
	return license.HTTPResponse{License: sealed}, nil
}

func handlerTrial(ctx context.Context, w http.ResponseWriter, mid license.MachineId) (license.HTTPResponse, error) {
	// Create the trial license that we'll insert if one doesn't already exist
	trialLicNew := trialLicenseCreate(mid)

	var trialLic license.DBTrialLicense
	trialLicRef := db.Collection(TrialsCollection).Doc(string(mid))
	err := db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Get the license
		trialLicDoc, err := tx.Get(trialLicRef)
		// If we failed to get the license because it didn't exist, create it
		// If we failed for any other reason, return the error
		if err != nil {
			if status.Code(err) == codes.NotFound {
				// Create the license
				err = tx.Create(trialLicRef, trialLicNew)
				if err != nil {
					return fmt.Errorf("tx.Create() failed: %w", err)
				}
				// Remember the license that we created via the `lic` variable that resides outside
				// of our scope, so it can be sent to the client.
				trialLic = trialLicNew
				return nil
			} else {
				return fmt.Errorf("tx.Get() failed: %w", err)
			}
		}

		// The license exists; write it to `trialLic`
		if err = trialLicDoc.DataTo(&trialLic); err != nil {
			return fmt.Errorf("trialLicDoc.DataTo() failed: %w", err)
		}
		return nil
	})

	if err != nil {
		return license.HTTPResponse{}, fmt.Errorf("db.RunTransaction failed: %w", err)
	}

	// Check if license is expired
	// If so, we return an error rather than return a signed license, otherwise the client
	// could more easily get the license and rollback the time of their machine.
	// The way we implemented debase, it deletes the license on disk after it expires, and
	// since we (the server) no longer supply the license after it expires, the license is
	// hopefully unrecoverable.
	expiration := time.Unix(trialLic.Expiration, 0)
	if time.Now().After(expiration) {
		return license.HTTPResponse{Error: "trial expired"}, nil
	}

	sealed, err := sealedLicense(license.HTTPLicense{
		MachineId:  trialLic.MachineId,
		Version:    trialLic.Version,
		Expiration: trialLic.Expiration,
	})
	if err != nil {
		return license.HTTPResponse{}, fmt.Errorf("sealedLicense failed: %w", err)
	}
	return license.HTTPResponse{License: sealed}, nil
}

func handler(w http.ResponseWriter, r *http.Request) error {
	ctx := r.Context()

	var req license.HTTPRequest
	err := json.NewDecoder(r.Body).Decode(&req)
	if err != nil {
		return fmt.Errorf("json.NewDecoder failed: %w", err)
	}

	req, err = reqSanitize(req)
	if err != nil {
		return fmt.Errorf("reqSanitize failed: %w", err)
	}

	var resp license.HTTPResponse
	if req.UserId != "" {
		// License request
		resp, err = handlerLicense(ctx, w, req)
		if err != nil {
			return fmt.Errorf("handlerLicense failed: %w", err)
		}

	} else {
		// Trial request
		resp, err = handlerTrial(ctx, w, req.MachineId)
		if err != nil {
			return fmt.Errorf("handlerTrial failed: %w", err)
		}
	}

	err = json.NewEncoder(w).Encode(&resp)
	if err != nil {
		return fmt.Errorf("json.NewEncoder(w).Encode failed: %w", err)
	}

	return nil
}

// Entry point
func DebaseLicenseServer(w http.ResponseWriter, r *http.Request) {
	err := handler(w, r)
	if err != nil {
		log.Printf("Error: %v", err)
		// TODO: don't return the actual error to the client, for security and to protect our licensing scheme
		http.Error(w, fmt.Sprintf("Error: %v", err), http.StatusBadRequest) // Intentionally not divulging the error in the response
		return
	}
}
