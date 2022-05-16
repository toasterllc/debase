package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"

	"github.com/stripe/stripe-go/v72"
	"github.com/stripe/stripe-go/v72/webhook"
)

type ReplyPurchaseFinishWebhook struct{}

func purchaseFinishWebhookErr(logFmt string, logArgs ...interface{}) Reply {
	log.Printf("purchase-finish-webhook error: "+logFmt, logArgs...)
	// We always return nil on error, to trigger a 400
	return nil
}

func endpointPurchaseFinishWebhook(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	const BodyLenMax = int64(65536)

	r.Body = http.MaxBytesReader(w, r.Body, BodyLenMax)
	payload, err := ioutil.ReadAll(r.Body)
	if err != nil {
		return purchaseFinishWebhookErr("ioutil.ReadAll() failed: %v", err)
	}

	// Verify event signature
	event, err := webhook.ConstructEvent(payload, r.Header.Get("Stripe-Signature"), StripePurchaseFinishWebhookSecret)
	if err != nil {
		return purchaseFinishWebhookErr("webhook.ConstructEvent() failed: %v", err)
	}

	// Verify that the event is the payment-succeeded event
	if event.Type != "payment_intent.succeeded" {
		return purchaseFinishWebhookErr("invalid event.Type: %v", event.Type)
	}

	// Unmarshal the PaymentIntent
	var pi stripe.PaymentIntent
	err = json.Unmarshal(event.Data.Raw, &pi)
	if err != nil {
		return purchaseFinishWebhookErr("json.Unmarshal() failed: %v", err)
	}

	payMethod := pi.PaymentMethod
	if payMethod == nil {
		return purchaseFinishWebhookErr("PaymentIntent.PaymentMethod nil")
	}

	log.Printf("payMethod: %v", payMethod)
	return ReplyPurchaseFinishWebhook{}
}
