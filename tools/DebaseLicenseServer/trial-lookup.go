package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"
	"time"

	"cloud.google.com/go/firestore"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"heytoaster.com/DebaseLicenseServer/license"
)

type CommandTrialLookup struct {
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

const TrialDuration = 7 * 24 * time.Hour
const TrialIssueCountMax = 25

var TrialExpiredErr = errors.New("The existing trial has already expired.")

func trialCreate(minfo license.MachineInfo) *license.DBTrial {
	expiration := time.Now().Add(TrialDuration)
	return &license.DBTrial{
		Version:    DebaseVersion,
		Expiration: expiration.Unix(),
		Machine:    *license.DBMachineCreate(minfo),
	}
}

func trialErr(userErr error, logFmt string, logArgs ...interface{}) Reply {
	log.Printf("trial-lookup error: "+logFmt, logArgs...)
	if userErr == nil {
		userErr = UnknownErr
	}
	return &ReplyLicenseLookup{Error: userErr.Error()}
}

func endpointTrialLookup(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	var cmd CommandTrialLookup
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return trialErr(LicenseNotFoundErr, "invalid command")
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return trialErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %v", err)
	}

	minfo := license.MachineInfoForString(cmd.MachineInfo)

	// Create the trial license that we'll insert if one doesn't already exist
	trialNew := trialCreate(minfo)

	var trial *license.DBTrial
	trialRef := Db.Collection(TrialsCollection).Doc(string(mid))
	err = Db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
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
		// contention, resulting in Db.RunTransaction() failing. Capping the IssueCount fixes
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
		return trialErr(LicenseNotFoundErr, "Db.RunTransaction() failed: %v", err)
	}

	// Check if license is expired
	// If so, we return an error rather than return a signed license, otherwise the client
	// could more easily get the signed license and rollback the time of their machine.
	// The way we implemented debase, it deletes the license on disk after it expires, and
	// since we (the server) no longer supply the license after it expires, the license is
	// hopefully unrecoverable.
	expiration := time.Unix(trial.Expiration, 0)
	if time.Now().After(expiration) {
		return trialErr(TrialExpiredErr, "trial expired for MachineId=%v", mid)
	}

	sealed := sealedLicense(license.HTTPLicense{
		MachineId:  string(mid),
		Version:    trial.Version,
		Expiration: trial.Expiration,
	})

	return &ReplyLicenseLookup{
		License: sealed,
	}
}
