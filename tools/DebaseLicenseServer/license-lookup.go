package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"

	"cloud.google.com/go/firestore"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"heytoaster.com/DebaseLicenseServer/license"
)

const MachineCountMax = 5

var LicenseNotFoundErr = errors.New("No matching license was found.")
var MachineLimitErr = errors.New("The maximum number of machines has already been registered for this license code.")

type CommandLicenseLookup struct {
	Email       string `json:"email"`
	LicenseCode string `json:"licenseCode"`
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

type ReplyLicenseLookup struct {
	Error   string                    `json:"error"`
	License license.HTTPSealedLicense `json:"license"`
}

func licenseErr(userErr error, logFmt string, logArgs ...interface{}) Reply {
	log.Printf("License lookup error: "+logFmt, logArgs...)
	if userErr == nil {
		userErr = UnknownErr
	}
	return &ReplyLicenseLookup{Error: userErr.Error()}
}

func handlerLicenseLookup(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	var cmd CommandLicenseLookup
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "invalid command")
	}

	email, err := license.EmailForString(cmd.Email)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.EmailForString() failed: %v", err)
	}

	mid, err := license.MachineIdForString(cmd.MachineId)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.MachineIdForString() failed: %v", err)
	}

	minfo := license.MachineInfoForString(cmd.MachineInfo)

	code, err := license.LicenseCodeForString(cmd.LicenseCode)
	if err != nil {
		return licenseErr(LicenseNotFoundErr, "license.LicenseCodeForString() failed: %v", err)
	}

	uid := license.DBUserIdForEmail(DebaseProductId, email)

	var lic *license.DBLicense
	var userErr error
	licsRef := Db.Collection(LicensesCollection).Doc(string(uid))
	err = Db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
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
		return licenseErr(userErr, "Db.RunTransaction() failed: %v", err)
	}

	sealed := sealedLicense(license.HTTPLicense{
		Email:       string(email),
		LicenseCode: string(code),
		MachineId:   string(mid),
		Version:     lic.Version,
	})

	return &ReplyLicenseLookup{
		License: sealed,
	}
}
