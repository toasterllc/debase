package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"errors"
	"log"
	"net/http"

	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/paymentintent"
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
	pi, err := paymentintent.Get(cmd.PaymentIntentId, nil)
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

	return &ReplyCompletePurchase{
		LicenseEmail: string(email),
		LicenseCodes: nil,
	}
}
