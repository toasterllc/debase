package DebaseLicenseServer

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io"
	"log"
	"net/http"

	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/paymentintent"
)

const LicensePrice = 1000 // USD cents
const LicenseCountMax = 10

var InvalidLicenseCountErr = errors.New("Invalid license count.")

type CommandPaymentIntent struct {
	LicenseCount int `json:"licenseCount"`
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

	if cmd.LicenseCount < 1 || cmd.LicenseCount > LicenseCountMax {
		return paymentIntentErr(InvalidLicenseCountErr, "invalid license count: %v", cmd.LicenseCount)
	}

	price := int64(cmd.LicenseCount) * LicensePrice
	pi, err := paymentintent.New(&stripe.PaymentIntentParams{
		Amount:   stripe.Int64(price),
		Currency: stripe.String(string(stripe.CurrencyUSD)),
		AutomaticPaymentMethods: &stripe.PaymentIntentAutomaticPaymentMethodsParams{
			Enabled: stripe.Bool(true),
		},
	})

	if err != nil {
		return paymentIntentErr(UnknownErr, "paymentintent.New failed: %v", err)
	}

	return &ReplyPaymentIntent{
		ClientSecret: pi.ClientSecret,
	}
}

func writeJSON(w http.ResponseWriter, v interface{}) {
	var buf bytes.Buffer
	if err := json.NewEncoder(&buf).Encode(v); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		log.Printf("json.NewEncoder.Encode: %v", err)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	if _, err := io.Copy(w, &buf); err != nil {
		log.Printf("io.Copy: %v", err)
		return
	}
}
