package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"cloud.google.com/go/firestore"
	"github.com/heytoaster/sesgo"
	"heytoaster.com/DebaseLicenseServer/license"
)

const ToasterSupportEmail = "Toaster Support <support@heytoaster.com>"

const LicenseEmailRateLimit = 10 * time.Minute
const LicenseEmailSubject = "Debase License"
const LicenseEmailBody = `
Hello, please find your debase license codes below.

    %v

If you have any questions please respond to this email.

Toaster Support
`

type CommandLicenseEmailSend struct {
	Email string `json:"email"`
}

type ReplyLicenseEmailSend struct{}

func emailErr(logFmt string, logArgs ...interface{}) Reply {
	log.Printf("License email send error: "+logFmt, logArgs...)
	// We always return a success reply, to ensure we don't divulge
	// any info about the existence of users
	return &ReplyLicenseEmailSend{}
}

func handlerLicenseEmailSend(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	var cmd CommandLicenseEmailSend
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return emailErr("invalid command payload")
	}

	email, err := license.EmailForString(cmd.Email)
	if err != nil {
		return emailErr("license.EmailForString() failed: %v", err)
	}
	uid := license.DBUserIdForEmail(DebaseProductId, email)

	var lics *license.DBLicenses
	licsRef := Db.Collection(LicensesCollection).Doc(string(uid))
	err = Db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
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
		return emailErr("Db.RunTransaction() failed: %v", err)
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
		return emailErr("sesgo.SendEmail() failed: %v", err)
	}

	return &ReplyLicenseEmailSend{}
}
