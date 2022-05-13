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
	"os"
	"path"
	"strings"
	"time"
	"unsafe"

	// "github.com/heytoaster/sesgo"
	"github.com/heytoaster/sesgo"
	"heytoaster.com/DebaseLicenseServer/license"

	"cloud.google.com/go/firestore"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// #cgo CFLAGS: -DDebaseLicenseServer=1
// #include "Debase.h"
import "C"

type Reply interface{}

var ReplyEmpty Reply = struct{}{}

const LicensesCollection = "debase-licenses"
const TrialsCollection = "debase-trials"
const TrialDuration = 7 * 24 * time.Hour
const TrialIssueCountMax = 25
const MachineCountMax = 3
const ToasterSupportEmail = "Toaster Support <support@heytoaster.com>"
const LicenseEmailSubject = "Debase License"
const LicenseEmailBody = `
Hello, please find your debase license codes below.

    %v

If you have any questions please respond to this email.

Toaster Support
`
const LicenseEmailRateLimit = 10 * time.Minute

const (
	EndpointLicenseLookup    = "license-lookup"
	EndpointTrialLookup      = "trial-lookup"
	EndpointLicenseEmailSend = "license-email-send"
)

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

func trialCreate(minfo license.MachineInfo) *license.DBTrial {
	expiration := time.Now().Add(TrialDuration)
	return &license.DBTrial{
		Version:    uint32(C.DebaseVersion),
		Expiration: expiration.Unix(),
		Machine:    *license.DBMachineCreate(minfo),
	}
}

func sealedLicense(lic license.HTTPLicense) license.HTTPSealedLicense {
	// Convert `lic` to json
	payload, err := json.Marshal(lic)
	if err != nil {
		panic("json.Marshal failed on license.HTTPLicense")
	}

	seed := *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
	key := ed25519.NewKeyFromSeed(seed[:])
	sig := ed25519.Sign(key, payload)
	return license.HTTPSealedLicense{
		Payload:   string(payload),
		Signature: hex.EncodeToString(sig),
	}
}

func licenseErr(userErr error, logErr string, logArgs ...interface{}) (Reply, error) {
	var reply Reply
	if userErr != nil {
		reply = &ReplyLicenseLookup{Error: userErr.Error()}
	}
	return reply, fmt.Errorf(logErr, logArgs...)
}

func handlerLicenseLookup(ctx context.Context, w http.ResponseWriter, r *http.Request) (Reply, error) {
	var cmd CommandLicenseLookup
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "invalid command")
	}

	email, err := license.EmailForString(cmd.Email)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.EmailForString() failed: %w", err)
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %w", err)
	}

	minfo := license.MachineInfoForString(cmd.MachineInfo)

	code, err := license.LicenseCodeForString(cmd.LicenseCode)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.LicenseCodeForString() failed: %w", err)
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
		return licenseErr(userErr, "db.RunTransaction() failed: %w", err)
	}

	sealed := sealedLicense(license.HTTPLicense{
		Email:       string(email),
		LicenseCode: string(code),
		MachineId:   string(mid),
		Version:     lic.Version,
	})

	return &ReplyLicenseLookup{
		License: sealed,
	}, nil
}

func handlerTrialLookup(ctx context.Context, w http.ResponseWriter, r *http.Request) (Reply, error) {
	var cmd CommandTrialLookup
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "invalid command")
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %w", err)
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
		return licenseErr(nil, "db.RunTransaction() failed: %w", err)
	}

	// Check if license is expired
	// If so, we return an error rather than return a signed license, otherwise the client
	// could more easily get the signed license and rollback the time of their machine.
	// The way we implemented debase, it deletes the license on disk after it expires, and
	// since we (the server) no longer supply the license after it expires, the license is
	// hopefully unrecoverable.
	expiration := time.Unix(trial.Expiration, 0)
	if time.Now().After(expiration) {
		return licenseErr(TrialExpiredErr, "trial expired for MachineId=%v", mid)
	}

	sealed := sealedLicense(license.HTTPLicense{
		MachineId:  string(mid),
		Version:    trial.Version,
		Expiration: trial.Expiration,
	})

	return &ReplyLicenseLookup{
		License: sealed,
	}, nil
}

func handlerLicenseEmailSend(ctx context.Context, w http.ResponseWriter, r *http.Request) error {
	var cmd CommandLicenseEmailSend
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return fmt.Errorf("invalid command payload")
	}

	email, err := license.EmailForString(cmd.Email)
	if err != nil {
		return fmt.Errorf("license.EmailForString() failed: %w", err)
	}
	uid := license.DBUserIdForEmail(DebaseProductId, email)

	var lics *license.DBLicenses
	licsRef := db.Collection(LicensesCollection).Doc(string(uid))
	err = db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Transactions can run multiple times, where the last one wins.
		// So make sure that our output vars are cleared by default, so they don't
		// contain values from a previous transaction
		lics = nil

		licsDoc, err := tx.Get(licsRef)
		if err != nil {
			return fmt.Errorf("tx.Get() failed for UserId=%v: %w", uid, err)
		}

		err = licsDoc.DataTo(&lics)
		if err != nil {
			return fmt.Errorf("licsDoc.DataTo() failed: %w", err)
		}

		now := time.Now()
		lastLicenseEmailTimestamp := time.Unix(lics.LastLicenseEmailTimestamp, 0)

		if now.Sub(lastLicenseEmailTimestamp) < LicenseEmailRateLimit {
			return fmt.Errorf("rejecting license-email-send request due to rate limit")
		}

		// Update time of last email being sent
		lics.LastLicenseEmailTimestamp = now.Unix()

		err = tx.Set(licsRef, lics)
		if err != nil {
			return fmt.Errorf("tx.Set() failed for UserId=%v", uid)
		}

		return nil
	})

	if err != nil {
		return fmt.Errorf("db.RunTransaction() failed: %w", err)
	}

	awsAccessKey := os.Getenv("AWS_ACCESS_KEY_ID")
	awsSecretKey := os.Getenv("AWS_SECRET_ACCESS_KEY")
	codes := []string{}
	for code := range lics.Licenses {
		codes = append(codes, string(code))
	}
	body := fmt.Sprintf(LicenseEmailBody, strings.Join(codes, ", "))
	err = sesgo.SendEmail(awsAccessKey, awsSecretKey, lics.Email, ToasterSupportEmail, LicenseEmailSubject, body)
	if err != nil {
		return fmt.Errorf("sesgo.SendEmail() failed: %w", err)
	}

	return nil
}

func handler(w http.ResponseWriter, r *http.Request) (Reply, error) {
	ctx := r.Context()

	endpoint := path.Base(r.URL.Path)
	switch endpoint {
	case EndpointLicenseLookup:
		return handlerLicenseLookup(ctx, w, r)
	case EndpointTrialLookup:
		return handlerTrialLookup(ctx, w, r)
	case EndpointLicenseEmailSend:
		// We always return ReplyEmpty (==success) because we don't want to give any indication
		// as to whether the email address is being used
		return ReplyEmpty, handlerLicenseEmailSend(ctx, w, r)
	default:
		return nil, fmt.Errorf("invalid endpoint: %v", endpoint)
	}
}

// Entry point
func DebaseLicenseServer(w http.ResponseWriter, r *http.Request) {
	reply, errLog := handler(w, r)

	// Log the error before doing anything else
	if errLog != nil {
		log.Printf("Error: %v", errLog)
	}

	// If there was a reply, write it and return (which implicitly triggers an HTTP 200).
	// A 200 doesn't mean we succeeded, it just means that we returned a valid json response
	// to the client.
	if reply != nil {
		err := json.NewEncoder(w).Encode(reply)
		if err != nil {
			log.Printf("json.NewEncoder().Encode() failed: %w", err)
		}

	} else {
		// Deliberately not divulging the error in the response, for security and to protect
		// our licensing scheme
		// The actual errors are logged via log.Printf()
		http.Error(w, "Error", http.StatusBadRequest)
	}
}
