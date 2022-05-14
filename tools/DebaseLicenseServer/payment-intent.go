package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"errors"
	"log"
	"net/http"
	"strconv"

	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/paymentintent"
)

const LicenseCountKey = "licenseCount"
const LicensePrice = 1000 // USD cents
const LicenseCountMin = 1
const LicenseCountMax = 10

var InvalidLicenseCountErr = errors.New("Invalid license count.")

type CommandPaymentIntent struct {
	PaymentIntentId string `json:"paymentIntentId"`
	LicenseCount    int    `json:"licenseCount"`
}

type ReplyPaymentIntent struct {
	Error        string `json:"error"`
	ClientSecret string `json:"clientSecret"`
}

func paymentIntentErr(userErr error, logFmt string, logArgs ...interface{}) Reply {
	log.Printf("Payment intent error: "+logFmt, logArgs...)
	if userErr == nil {
		userErr = UnknownErr
	}
	return &ReplyPaymentIntent{Error: userErr.Error()}
}

func handlePaymentIntent(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	var cmd CommandPaymentIntent
	err := json.NewDecoder(r.Body).Decode(&cmd)
	if err != nil {
		return paymentIntentErr(UnknownErr, "invalid command payload")
	}

	if cmd.LicenseCount < LicenseCountMin || cmd.LicenseCount > LicenseCountMax {
		return paymentIntentErr(InvalidLicenseCountErr, "invalid license count: %v", cmd.LicenseCount)
	}

	price := int64(cmd.LicenseCount) * LicensePrice
	params := stripe.PaymentIntentParams{
		Amount:   stripe.Int64(price),
		Currency: stripe.String(string(stripe.CurrencyUSD)),
		AutomaticPaymentMethods: &stripe.PaymentIntentAutomaticPaymentMethodsParams{
			Enabled: stripe.Bool(true),
		},
	}
	params.AddMetadata(LicenseCountKey, strconv.Itoa(cmd.LicenseCount))

	var pi *stripe.PaymentIntent
	if cmd.PaymentIntentId == "" {
		// Create a new PaymentIntent because it didn't already exist
		pi, err = paymentintent.New(&params)
		if err != nil {
			return paymentIntentErr(UnknownErr, "paymentintent.New failed: %v", err)
		}

	} else {
		// Update existing PaymentIntent with the new params
		pi, err = paymentintent.Update(cmd.PaymentIntentId, &params)
		if err != nil {
			return paymentIntentErr(UnknownErr, "paymentintent.Update failed: %v", err)
		}
	}

	return &ReplyPaymentIntent{
		ClientSecret: pi.ClientSecret,
	}
}
