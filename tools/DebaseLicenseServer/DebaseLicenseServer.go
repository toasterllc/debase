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
	"time"
	"unsafe"

	// "github.com/heytoaster/sesgo"
	"heytoaster.com/DebaseLicenseServer/license"

	"cloud.google.com/go/firestore"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// #cgo CFLAGS: -DDebaseLicenseServer=1
// #include "Debase.h"
import "C"

const LicensesCollection = "debase-licenses"
const TrialsCollection = "debase-trials"
const TrialDuration = 7 * 24 * time.Hour
const TrialIssueCountMax = 25
const MachineCountMax = 3

var DebaseProductId = C.GoString(C.DebaseProductId)

var LicenseNotFoundErr = errors.New("No matching license was found.")
var MachineLimitErr = errors.New("The maximum number of machines has already been registered for this license code.")
var TrialExpiredErr = errors.New("The existing trial has already expired.")

var db *firestore.Client

func init() {
	var err error
	db, err = firestore.NewClient(context.Background(), firestore.DetectProjectID)
	if err != nil {
		log.Fatalf("firestore.NewClient() failed: %v", err)
	}
}

// func reqSanitize(req license.HTTPRequest) (license.HTTPRequest, error) {
//     var err error
//     req.MachineId, err = machineIdSanitize(req.MachineId)
//     if err != nil {
//         return license.HTTPRequest{}, fmt.Errorf("invalid machine id (%v): %w", req.MachineId, err)
//     }
//
//     if req.UserId != "" {
//         req.UserId, err = userIdSanitize(req.UserId)
//         if err != nil {
//             return license.HTTPRequest{}, fmt.Errorf("invalid user id (%v): %w", req.UserId, err)
//         }
//     }
//
//     if req.LicenseCode != "" {
//         req.LicenseCode, err = licenseCodeSanitize(req.LicenseCode)
//         if err != nil {
//             return license.HTTPRequest{}, fmt.Errorf("invalid license code (%v): %w", req.UserId, err)
//         }
//     }
//
//     return req, nil
// }

func trialCreate(minfo license.MachineInfo) *license.DBTrial {
	expiration := time.Now().Add(TrialDuration)
	return &license.DBTrial{
		Version:    uint32(C.DebaseVersion),
		Expiration: expiration.Unix(),
		Machine:    *license.DBMachineCreate(minfo),
	}
}

func sealedLicense(lic license.HTTPLicense) (license.HTTPSealedLicense, error) {
	// Convert `lic` to json
	payload, err := json.Marshal(lic)
	if err != nil {
		return license.HTTPSealedLicense{}, fmt.Errorf("json.Marshal() failed: %w", err)
	}

	seed := *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
	key := ed25519.NewKeyFromSeed(seed[:])
	sig := ed25519.Sign(key, payload)
	return license.HTTPSealedLicense{
		Payload:   string(payload),
		Signature: hex.EncodeToString(sig),
	}, nil
}

func replyErr(userErr error, logErr string, logArgs ...interface{}) (*Reply, error) {
	var reply *Reply
	if userErr != nil {
		reply = &Reply{Error: userErr.Error()}
	}
	return reply, fmt.Errorf(logErr, logArgs...)
}

func handlerLicenseLookup(ctx context.Context, w http.ResponseWriter, p json.RawMessage) (*Reply, error) {
	var cmd CommandLicenseLookup
	err := json.Unmarshal(p, &cmd)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "invalid command payload", string(p))
	}

	email, err := license.EmailForString(cmd.Email)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "license.EmailForString() failed: %w", err)
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %w", err)
	}

	minfo := license.MachineInfoForString(cmd.MachineInfo)

	code, err := license.LicenseCodeForString(cmd.LicenseCode)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "license.LicenseCodeForString() failed: %w", err)
	}

	uid := license.DBUserIdForEmail(DebaseProductId, email)

	var lic *license.DBLicense
	var userErr error
	licsRef := db.Collection(LicensesCollection).Doc(string(uid))
	err = db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Transactions can run multiple times, where the last one wins.
		// So make sure that our output vars are cleared by default, so they don't
		// contain values from a previous transaction
		lic = nil
		userErr = nil

		licsDoc, err := tx.Get(licsRef)
		if err != nil {
			if status.Code(err) == codes.NotFound {
				userErr = LicenseNotFoundErr
			}
			return fmt.Errorf("tx.Get() failed for UserId=%v LicenseCode=%v: %w", uid, code, err)
		}

		var lics license.DBLicenses
		err = licsDoc.DataTo(&lics)
		if err != nil {
			return fmt.Errorf("licsDoc.DataTo() failed: %w", err)
		}

		// Find the license for the given license code, or return an error if one doesn't exist
		lic = lics.Licenses[code]
		if lic == nil {
			userErr = LicenseNotFoundErr
			return fmt.Errorf("no matching license for UserId=%v LicenseCode=%v", uid, code)
		}

		machine := lic.Machines[mid]
		if machine == nil {
			// // We found a license for the given license code
			// // If the given machine id already exists in the license, then we're done
			// for _, x := range userLic.MachineIds {
			//     if x == mid {
			//         return nil
			//     }
			// }

			// Otherwise, the machine id doesn't exist in the license, so we need to add it
			// Bail if we don't have any free machine id slots
			if len(lic.Machines) >= MachineCountMax {
				userErr = MachineLimitErr
				return fmt.Errorf("max machines already reached for UserId=%v LicenseCode=%v", uid, code)
			}

			lic.Machines[mid] = license.DBMachineCreate(minfo)

		} else {
			machine.IssueCount++
		}

		err = tx.Set(licsRef, lics)
		if err != nil {
			return fmt.Errorf("tx.Set() failed for UserId=%v LicenseCode=%v", uid, code)
		}

		return nil
	})

	if err != nil {
		return replyErr(userErr, "db.RunTransaction() failed: %w", err)
	}

	sealed, err := sealedLicense(license.HTTPLicense{
		Email:       string(email),
		LicenseCode: string(code),
		MachineId:   string(mid),
		Version:     lic.Version,
	})
	if err != nil {
		return replyErr(nil, "sealedLicense() failed: %w", err)
	}

	return &Reply{Payload: ReplyLicenseLookup(sealed)}, nil
}

func handlerTrialLookup(ctx context.Context, w http.ResponseWriter, p json.RawMessage) (*Reply, error) {
	var cmd CommandTrialLookup
	err := json.Unmarshal(p, &cmd)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "invalid command payload", string(p))
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return replyErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %w", err)
	}

	minfo := license.MachineInfoForString(cmd.MachineInfo)

	// Create the trial license that we'll insert if one doesn't already exist
	trialNew := trialCreate(minfo)

	var trial *license.DBTrial
	trialRef := db.Collection(TrialsCollection).Doc(string(mid))
	err = db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Get the license
		trialDoc, err := tx.Get(trialRef)
		// If we failed to get the license because it didn't exist, create it
		// If we failed for any other reason, return the error
		if err != nil {
			if status.Code(err) == codes.NotFound {
				// Create the license
				err = tx.Create(trialRef, trialNew)
				if err != nil {
					return fmt.Errorf("tx.Create() failed: %w", err)
				}
				// Remember the license that we created via the `trial` variable that resides outside
				// of our scope, so it can be sent to the client.
				trial = trialNew
				return nil
			} else {
				return fmt.Errorf("tx.Get() failed: %w", err)
			}
		}

		// The license exists; read it into `trial`
		if err = trialDoc.DataTo(&trial); err != nil {
			return fmt.Errorf("trialDoc.DataTo() failed: %w", err)
		}

		// Update the trial's machine's IssueCount, but limit it to `TrialIssueCountMax`
		// This limiting is necessary to reduce contention in the case where many machines are
		// using the same machineId. This should ideally never happen, but our machine-id
		// fingerprinting logic isn't perfect.
		// With many machines potentially using the same machine-id, they'll all reference the
		// same Firestore document, and lots of machines modifying the same document introduces
		// contention, resulting in db.RunTransaction() failing. Capping the IssueCount fixes
		// this contention since we'll stop trying to modify the document once the IssueCount
		// reaches its max value.
		if trial.Machine.IssueCount < TrialIssueCountMax {
			trial.Machine.IssueCount++
			err = tx.Set(trialRef, trial)
			if err != nil {
				return fmt.Errorf("tx.Set() failed for MachineId=%v", mid)
			}
		}

		return nil
	})

	if err != nil {
		return replyErr(nil, "db.RunTransaction() failed: %w", err)
	}

	// Check if license is expired
	// If so, we return an error rather than return a signed license, otherwise the client
	// could more easily get the signed license and rollback the time of their machine.
	// The way we implemented debase, it deletes the license on disk after it expires, and
	// since we (the server) no longer supply the license after it expires, the license is
	// hopefully unrecoverable.
	expiration := time.Unix(trial.Expiration, 0)
	if time.Now().After(expiration) {
		return replyErr(TrialExpiredErr, "trial expired for MachineId=%v", mid)
	}

	sealed, err := sealedLicense(license.HTTPLicense{
		MachineId:  string(mid),
		Version:    trial.Version,
		Expiration: trial.Expiration,
	})
	if err != nil {
		return replyErr(nil, "sealedLicense() failed: %w", err)
	}

	return &Reply{Payload: ReplyTrialLookup(sealed)}, nil
}

func handlerLicenseEmailSend(ctx context.Context, w http.ResponseWriter, p json.RawMessage) (*Reply, error) {
	var cmd CommandLicenseEmailSend
	err := json.Unmarshal(p, &cmd)
	if err != nil {
		return replyErr(nil, "invalid command payload", string(p))
	}

	return &Reply{Payload: ReplyLicenseEmailSend{}}, nil
}

func handler(w http.ResponseWriter, r *http.Request) (*Reply, error) {
	ctx := r.Context()

	var cmd Command
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return nil, fmt.Errorf("json.NewDecoder() failed: %w", err)
	}

	switch cmd.Type {
	case LicenseLookup:
		return handlerLicenseLookup(ctx, w, cmd.Payload)
	case TrialLookup:
		return handlerTrialLookup(ctx, w, cmd.Payload)
	case LicenseEmailSend:
		return handlerLicenseEmailSend(ctx, w, cmd.Payload)
	default:
		return nil, fmt.Errorf("invalid command type: %v", cmd.Type)
	}
}

// Entry point
func DebaseLicenseServer(w http.ResponseWriter, r *http.Request) {
	reply, err := handler(w, r)

	// Log the error before doing anything else
	if err != nil {
		log.Printf("Error: %v", err)
	}

	// If there was a reply, write it and return (which implicitly triggers an HTTP 200).
	// A 200 doesn't mean we succeeded, it just means that we returned a valid json response
	// to the client.
	if reply != nil {
		err = json.NewEncoder(w).Encode(reply)
		if err != nil {
			log.Printf("json.NewEncoder().Encode() failed: %w", err)
		}
		return
	}

	// Deliberately not divulging the error in the response, for security and to protect
	// our licensing scheme
	// The actual errors are logged via log.Printf()
	http.Error(w, "Error", http.StatusBadRequest)
}
