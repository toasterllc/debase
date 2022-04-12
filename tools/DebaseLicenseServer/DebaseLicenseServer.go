package DebaseLicenseServer

import (
	"context"
	"crypto/ed25519"
	"encoding/hex"
	"encoding/json"
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

func registerCodeSanitize(code license.RegisterCode) (license.RegisterCode, error) {
	str := strings.ToLower(string(code))
	regex := regexp.MustCompile(`^[a-z0-9]+$`)
	if !regex.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}
	return license.RegisterCode(str), nil
}

func reqSanitize(req license.Request) (license.Request, error) {
	var err error
	req.MachineId, err = machineIdSanitize(req.MachineId)
	if err != nil {
		return license.Request{}, fmt.Errorf("invalid machine id (%v): %w", req.MachineId, err)
	}

	if req.UserId != "" {
		req.UserId, err = userIdSanitize(req.UserId)
		if err != nil {
			return license.Request{}, fmt.Errorf("invalid user id (%v): %w", req.UserId, err)
		}
	}

	if req.RegisterCode != "" {
		req.RegisterCode, err = registerCodeSanitize(req.RegisterCode)
		if err != nil {
			return license.Request{}, fmt.Errorf("invalid register code (%v): %w", req.UserId, err)
		}
	}

	return req, nil

	// // Validate the machine id, which is always required
	// if len(req.MachineId) != MachineIdLen {
	//     return license.Request{}, fmt.Errorf("invalid request")
	// }
	//
	//     req.MachineId = strings.ToLower(req.MachineId)
	//     validHex := regexp.MustCompile(`^[a-f0-9]+$`)
	//     if !validHex.MatchString(req.MachineId) {
	//         return license.Request{}, fmt.Errorf("invalid request")
	//     }
	//
	// // Required
	// MachineId MachineId `json:"machineId"`
	// // Optional
	// UserId       UserId       `json:"userId"`
	// RegisterCode RegisterCode `json:"registerCode"`
}

func trialLicenseCreate(mid license.MachineId) license.License {
	expiration := time.Now().Add(TrialDuration)
	return license.License{
		MachineId:  mid,
		Version:    uint32(C.DebaseVersion),
		Expiration: expiration.Unix(),
	}
}

func sealedLicenseForLicense(lic license.License) (license.SealedLicense, error) {
	// Convert `lic` to json
	payload, err := json.Marshal(lic)
	if err != nil {
		return license.SealedLicense{}, fmt.Errorf("json.Marshal failed: %w", err)
	}

	seed := *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
	key := ed25519.NewKeyFromSeed(seed[:])
	sig := ed25519.Sign(key, payload)
	return license.SealedLicense{
		Payload:   string(payload),
		Signature: hex.EncodeToString(sig),
	}, nil
}

func handlerLicense(ctx context.Context, w http.ResponseWriter, req license.Request) (license.Response, error) {
	userLicsRef := db.Collection(LicensesCollection).Doc(string(req.UserId))
	userLicsDoc, err := userLicsRef.Get(ctx)
	if err != nil {
		return license.Response{}, fmt.Errorf("userLicsRef.Get failed: %w", err)
	}

	var userLics license.UserLicenses
	err = userLicsDoc.DataTo(&userLics)
	if err != nil {
		return license.Response{}, fmt.Errorf("userLicsDoc.DataTo failed: %w", err)
	}

	var match *license.License
	for _, lic := range userLics.Licenses {
		if lic.RegisterCode == req.RegisterCode {
			match = &lic
		}
	}

	if match == nil {
		return license.Response{Error: "no matching license"}, nil
	}

	return nil
}

func handlerTrial(ctx context.Context, w http.ResponseWriter, mid license.MachineId) (license.Response, error) {
	// type TrialRecord struct {
	//     license.MachineId     `json:"machineId"`
	//     license.SealedLicense `json:"license"`
	// }

	// // Create trial, convert to json
	// lic := trialLicenseCreate(mid)
	// payloadb, err := json.Marshal(lic)
	// if err != nil {
	//     return fmt.Errorf("json.Marshal failed: %w", err)
	// }
	// payload := string(payloadb)
	//
	// //     //
	// // h := sha512.New512_256()
	// // h.Write(licb)
	// // sigb := h.Sum(nil)
	//
	// key := ed25519.PrivateKey(*(*[]byte)(unsafe.Pointer(&C.DebaseKeyPrivate)))
	// sigb := ed25519.Sign(key, payloadb)
	// sig := hex.EncodeToString(sigb)
	// sealed := license.SealedLicense{
	//     Payload:   payload,
	//     Signature: sig,
	// }

	// Create the trial license that we'll insert if one doesn't already exist
	licNew := trialLicenseCreate(mid)

	var lic license.License
	licRef := db.Collection(TrialsCollection).Doc(string(mid))
	err := db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Get the license
		licDoc, err := tx.Get(licRef)
		// If we failed to get the license because it didn't exist, create it
		// If we failed for any other reason, return the error
		if err != nil {
			if status.Code(err) == codes.NotFound {
				// Create the license
				err = tx.Create(licRef, licNew)
				if err != nil {
					return fmt.Errorf("licRef.Create failed: %w", err)
				}
				// Remember the license that we created via the `lic` variable that resides outside
				// of our scope, so it can be sent to the client.
				lic = licNew
				return nil
			} else {
				return fmt.Errorf("tx.Get failed: %w", err)
			}
		}

		// The license exists; write it to `lic`
		if err = licDoc.DataTo(&lic); err != nil {
			return fmt.Errorf("licDoc.DataTo failed: %w", err)
		}
		return nil
	})

	if err != nil {
		return license.Response{}, fmt.Errorf("db.RunTransaction failed: %w", err)
	}

	// Check if license is expired
	// If so, we return an error rather than return a signed license, otherwise the client
	// could more easily get the license and rollback the time of their machine.
	// The way we implemented debase, it deletes the license on disk after it expires, and
	// since we (the server) no longer supply the license after it expires, the license is
	// hopefully unrecoverable.
	expiration := time.Unix(lic.Expiration, 0)
	if time.Now().After(expiration) {
		return license.Response{Error: "trial has expired"}, nil
	}

	sealed, err := sealedLicenseForLicense(lic)
	if err != nil {
		return license.Response{}, fmt.Errorf("sealedLicenseForLicense failed: %w", err)
	}
	return license.Response{License: sealed}, nil
}

func handler(w http.ResponseWriter, r *http.Request) error {
	ctx := r.Context()

	var req license.Request
	err := json.NewDecoder(r.Body).Decode(&req)
	if err != nil {
		return fmt.Errorf("json.NewDecoder failed: %w", err)
	}

	req, err = reqSanitize(req)
	if err != nil {
		return fmt.Errorf("reqSanitize failed: %w", err)
	}

	var resp license.Response
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
