package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"cloud.google.com/go/firestore"
	"github.com/heytoaster/sesgo"
	"heytoaster.com/DebaseLicenseServer/license"
)

const ToasterSupportEmail = "Toaster Support <support@heytoaster.com>"

const LicenseEmailRateLimit = 10 * time.Minute

const LicenseEmailReceiptSubject = "Debase License Receipt"
const LicenseEmailReminderSubject = "Debase License Reminder"

const LicenseEmailReceiptBodyFmt = `Hello, thanks for supporting debase! Please find your license information below.

%v:
    %v

%v:
    %v

Please respond to this email if you have any questions.

Toaster Support
heytoaster.com
`

const LicenseEmailReminderBodyFmt = `Hello, please find your debase license information below.

%v:
%v

%v:
%v

Please respond to this email if you have any questions.

Toaster Support
heytoaster.com
`

type CommandLicenseEmailSend struct {
	Email string `json:"email"`
}

type ReplyLicenseEmailSend struct{}

func sendLicenseCodesEmail(to, subject, bodyFmt, email string, codes []string) error {
	indent := "    "
	emailFieldName := "License email"
	codesFieldName := "License code"
	if len(codes) > 1 {
		codesFieldName = "License codes"
	}

	email = indent + email
	codesStr := indent
	for _, code := range codes {
		if codesStr != "" {
			codesStr += indent + "\n"
		}

		codesStr += string(code)
	}

	body := fmt.Sprintf(bodyFmt, emailFieldName, email, codesFieldName, codesStr)
	return sesgo.SendEmail(AWSAccessKey, AWSSecretKey, to, ToasterSupportEmail, subject, body)
}

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

	var codes []string
	for code := range lics.Licenses {
		codes = append(codes, string(code))
	}

	err = sendLicenseCodesEmail(lics.Email, LicenseEmailReminderSubject, LicenseEmailReminderBodyFmt, lics.Email, codes)
	if err != nil {
		return emailErr("sesgo.SendEmail() failed: %v", err)
	}

	return &ReplyLicenseEmailSend{}
}
