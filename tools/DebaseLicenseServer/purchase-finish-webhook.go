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

func purchaseFinishWebhookErr(w http.ResponseWriter, logFmt string, logArgs ...interface{}) Reply {
	log.Printf("purchase-finish-webhook error: "+logFmt, logArgs...)
	http.Error(w, "Error", http.StatusBadRequest)
	// We always return a nil Reply, because we handled the HTTP request ourself,
	// and don't want DebaseLicenseServer() to write anything more
	return nil
}

func endpointPurchaseFinishWebhook(ctx context.Context, w http.ResponseWriter, r *http.Request) Reply {
	const BodyLenMax = int64(65536)

	r.Body = http.MaxBytesReader(w, r.Body, BodyLenMax)
	payload, err := ioutil.ReadAll(r.Body)
	if err != nil {
		return purchaseFinishWebhookErr(w, "ioutil.ReadAll() failed: %v", err)
	}

	// Verify event signature
	event, err := webhook.ConstructEvent(payload, r.Header.Get("Stripe-Signature"), StripePurchaseFinishWebhookSecret)
	if err != nil {
		return purchaseFinishWebhookErr(w, "webhook.ConstructEvent() failed: %v", err)
	}

	// Verify that the event is the payment-succeeded event
	if event.Type != "payment_intent.succeeded" {
		return purchaseFinishWebhookErr(w, "invalid event.Type: %v", event.Type)
	}

	// Unmarshal the PaymentIntent
	var pi stripe.PaymentIntent
	err = json.Unmarshal(event.Data.Raw, &pi)
	if err != nil {
		return purchaseFinishWebhookErr(w, "json.Unmarshal() failed: %v", err)
	}

	// Respond to Stripe with 'success' / HTTP 200 before we do the purchaseFinish()
	// heavy lifting, because Stripe docs say:
	//
	// > Your endpoint must quickly return a successful status code (2xx) prior
	// > to any complex logic that could cause a timeout. For example, you must
	// > return a 200 response before updating a customer’s invoice as paid in
	// > your accounting system.
	//
	w.WriteHeader(http.StatusOK)

	_, _, err = purchaseFinish(ctx, pi.ID)
	if err != nil {
		return purchaseFinishWebhookErr(w, "purchaseFinish() failed: %v", err)
	}

	// We always return a nil Reply, because we handled the HTTP request ourself,
	// and don't want DebaseLicenseServer() to write anything more
	return nil
}
