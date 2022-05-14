package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"time"

	"cloud.google.com/go/firestore"
	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/paymentintent"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"heytoaster.com/DebaseLicenseServer/license"
)

type CommandCompletePurchase struct {
	PaymentIntentId string `json:"paymentIntentId"`
}

type ReplyCompletePurchase struct {
	Error        string   `json:"error"`
	LicenseEmail string   `json:"licenseEmail"`
	LicenseCodes []string `json:"licenseCodes"`
}

var PaymentUnsuccessfulErr = errors.New("The payment was unsuccessful.")
var InvalidEmailErr = errors.New("Invalid email address.")

func licenseCreate(paymentId string) *license.DBLicense {
	return &license.DBLicense{
		Version:   DebaseVersion,
		Timestamp: time.Now().Unix(),
		PaymentId: paymentId,
	}
}

// licensesForPaymentId(): return the DBLicense's that match a given payment id
func licensesForPaymentId(lics *license.DBLicenses, paymentId string) map[license.LicenseCode]*license.DBLicense {
	r := map[license.LicenseCode]*license.DBLicense{}
	for code, lic := range lics.Licenses {
		if lic.PaymentId == paymentId {
			r[code] = lic
		}
	}
	return r
}

func completePurchaseErr(userErr error, logFmt string, logArgs ...interface{}) Reply {
	log.Printf("Payment intent error: "+logFmt, logArgs...)
	if userErr == nil {
		userErr = UnknownErr
	}
	return &ReplyCompletePurchase{Error: userErr.Error()}
}

func handleCompletePurchase(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	var cmd CommandCompletePurchase
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return completePurchaseErr(UnknownErr, "invalid command payload")
	}

	// Get the PaymentIntent
	pid := cmd.PaymentIntentId
	pi, err := paymentintent.Get(pid, nil)
	if err != nil {
		return completePurchaseErr(UnknownErr, "paymentintent.Get failed: %v", err)
	}

	// Confirm that the payment succeeded
	if pi.Status != stripe.PaymentIntentStatusSucceeded {
		return completePurchaseErr(PaymentUnsuccessfulErr, "PaymentIntent.Status invalid: expected %v, got %v", stripe.PaymentIntentStatusSucceeded, pi.Status)
	}

	// Get the PaymentMethod
	payMethod := pi.PaymentMethod
	if payMethod == nil {
		return completePurchaseErr(UnknownErr, "PaymentIntent.PaymentMethod nil")
	}

	// Get the BillingDetails
	billing := payMethod.BillingDetails
	if billing == nil {
		return completePurchaseErr(UnknownErr, "PaymentMethod.BillingDetails nil")
	}

	// string -> license.Email
	email, err := license.EmailForString(billing.Email)
	if err != nil {
		return completePurchaseErr(InvalidEmailErr, "license.EmailForString failed: %v", err)
	}

	// license.Email -> license.DBUserId
	uid := license.DBUserIdForEmail(DebaseProductId, email)

	// Get the license count
	licenseCount, err := strconv.Atoi(pi.Metadata[LicenseCountKey])
	if err != nil {
		return completePurchaseErr(InvalidLicenseCountErr, "strconv.Atoi(license count = %v) failed: %v", pi.Metadata[LicenseCountKey], err)
	}

	if licenseCount < LicenseCountMin || licenseCount > LicenseCountMax {
		return completePurchaseErr(InvalidLicenseCountErr, "invalid license count: %v", licenseCount)
	}
	// // Create the trial license that we'll insert if one doesn't already exist
	// trialNew := trialCreate(minfo)
	//
	// var trial *license.DBTrial
	// trialRef := Db.Collection(TrialsCollection).Doc(string(mid))
	// err = Db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
	//     // Get the license
	//     trialDoc, err := tx.Get(trialRef)
	//     // If we failed to get the license because it didn't exist, create it
	//     // If we failed for any other reason, return the error
	//     if err != nil {
	//         if status.Code(err) == codes.NotFound {
	//             // Create the license
	//             err = tx.Create(trialRef, trialNew)
	//             if err != nil {
	//                 return fmt.Errorf("tx.Create() failed: %w", err)
	//             }
	//             // Remember the license that we created via the `trial` variable that resides outside
	//             // of our scope, so it can be sent to the client.
	//             trial = trialNew
	//             return nil
	//         } else {
	//             return fmt.Errorf("tx.Get() failed: %w", err)
	//         }
	//     }
	//
	//     // The license exists; read it into `trial`
	//     if err = trialDoc.DataTo(&trial); err != nil {
	//         return fmt.Errorf("trialDoc.DataTo() failed: %w", err)
	//     }
	//
	//     // Update the trial's machine's IssueCount, but limit it to `TrialIssueCountMax`
	//     // This limiting is necessary to reduce contention in the case where many machines are
	//     // using the same machineId. This should ideally never happen, but our machine-id
	//     // fingerprinting logic isn't perfect.
	//     // With many machines potentially using the same machine-id, they'll all reference the
	//     // same Firestore document, and lots of machines modifying the same document introduces
	//     // contention, resulting in Db.RunTransaction() failing. Capping the IssueCount fixes
	//     // this contention since we'll stop trying to modify the document once the IssueCount
	//     // reaches its max value.
	//     if trial.Machine.IssueCount < TrialIssueCountMax {
	//         trial.Machine.IssueCount++
	//         err = tx.Set(trialRef, trial)
	//         if err != nil {
	//             return fmt.Errorf("tx.Set() failed for MachineId=%v", mid)
	//         }
	//     }
	//
	//     return nil
	// })
	//
	// if err != nil {
	//     return trialErr(LicenseNotFoundErr, "Db.RunTransaction() failed: %v", err)
	// }

	// Pre-create double the number of license codes than we need (just in case we we have
	// a collision).
	// We may not need them at all if we've already created the licenses for the
	// PaymentIntent, but we'd like to avoid generating them within the db transaction
	var licenseCodesPool []license.LicenseCode
	for i := 0; i < 2*licenseCount; i++ {
		c, err := license.LicenseCodeGenerate()
		if err != nil {
			return completePurchaseErr(UnknownErr, "LicenseCodeGenerate() failed: %v", err)
		}
		licenseCodesPool = append(licenseCodesPool, c)
	}

	var licsForPayment map[license.LicenseCode]*license.DBLicense
	var licsForPaymentCreated bool
	var userErr error
	licsRef := Db.Collection(LicensesCollection).Doc(string(uid))
	err = Db.RunTransaction(ctx, func(ctx context.Context, tx *firestore.Transaction) error {
		// Transactions can run multiple times, where the last one wins.
		// So make sure that our output vars are cleared by default, so they don't
		// contain values from a previous transaction
		licsForPayment = nil
		licsForPaymentCreated = false
		userErr = nil

		// Get the `lics` DBLicenses for the user id, creating it if it doesn't already exist
		var lics license.DBLicenses
		licsDoc, err := tx.Get(licsRef)
		if err == nil {
			err = licsDoc.DataTo(&lics)
			if err != nil {
				return fmt.Errorf("licsDoc.DataTo() failed: %w", err)
			}

		} else if status.Code(err) == codes.NotFound {
			// Create the DBLicenses because it doesn't exist yet
			lics = license.DBLicenses{
				Email: string(email),
			}

		} else {
			return fmt.Errorf("tx.Get() failed for UserId=%v: %w", uid, err)
		}

		// Only create licenses for the payment id if none exist.
		// This is our safeguard against creating more licenses than the user paid for.
		// This is necessary because both the frontend and our Stripe webhook call this
		// code when the payment completes, so inevitably one of them will lose, and we
		// need to make sure we only act on the payment and create the necessarily
		// licenses once.
		licsForPayment = licensesForPaymentId(&lics, pid)
		if len(licsForPayment) == 0 {
			// The payment id doesn't exist in `lics`, so licenses haven't yet been created
			// for the successful payment.
			poolIdx := 0
			for i := 0; i < licenseCount; poolIdx++ {
				if poolIdx >= len(licenseCodesPool) {
					return fmt.Errorf("failed to create enough unique license codes (pool: %v)", licenseCodesPool)
				}

				licenseCode := licenseCodesPool[poolIdx]
				// Check for collision with existing licenses
				if lics.Licenses[licenseCode] != nil {
					continue
				}

				// Create the license
				lic := licenseCreate(pid)
				lics.Licenses[licenseCode] = lic
				licsForPayment[licenseCode] = lic
			}

			licsForPaymentCreated = true
		}

		err = tx.Set(licsRef, lics)
		if err != nil {
			return fmt.Errorf("tx.Set() failed for UserId=%v", uid)
		}

		return nil
	})

	if err != nil {
		return completePurchaseErr(userErr, "Db.RunTransaction() failed: %v", err)
	}

	// Collect the license codes for the payment id into a slice
	var licenseCodes []string
	for code := range licsForPayment {
		licenseCodes = append(licenseCodes, code)
	}

	// Send email if we created the licenses for the payment
	if licsForPaymentCreated {
		err := sendLicenseCodesEmail(string(email), LicenseEmailReceiptSubject, LicenseEmailReceiptBodyFmt, licenseCodes)
		if err != nil {
			// Not returning here! We don't want to fail this endpoint just because we couldn't send the email
			completePurchaseErr(UnknownErr, "sendLicenseCodesEmail() failed: %v", err)
		}
	}

	return &ReplyCompletePurchase{
		LicenseEmail: string(email),
		LicenseCodes: licenseCodes,
	}
}
