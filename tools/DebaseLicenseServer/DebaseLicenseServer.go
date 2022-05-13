package DebaseLicenseServer

import (
	"context"
	"encoding/json"
	"log"
	"net/http"
	"path"

	// "github.com/heytoaster/sesgo"

	"cloud.google.com/go/firestore"
)

const (
	EndpointLicenseLookup    = "license-lookup"
	EndpointTrialLookup      = "trial-lookup"
	EndpointLicenseEmailSend = "license-email-send"
)

func init() {
	var err error
	Db, err = firestore.NewClient(context.Background(), firestore.DetectProjectID)
	if err != nil {
		log.Fatalf("firestore.NewClient() failed: %v", err)
	}
}

func handler(w http.ResponseWriter, r *http.Request) Reply {
	ctx := r.Context()

	endpoint := path.Base(r.URL.Path)
	switch endpoint {
	case EndpointLicenseLookup:
		return handlerLicenseLookup(ctx, w, r)
	case EndpointTrialLookup:
		return handlerTrialLookup(ctx, w, r)
	case EndpointLicenseEmailSend:
		return handlerLicenseEmailSend(ctx, w, r)
	default:
		return nil
	}
}

// Entry point
func DebaseLicenseServer(w http.ResponseWriter, r *http.Request) {
	reply := handler(w, r)
	if reply == nil {
		// We deliberately don't divulge any error in the response, for security / to protect
		// our licensing scheme.
		// The actual errors are logged via log.Printf()
		http.Error(w, "Error", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	err := json.NewEncoder(w).Encode(reply)
	if err != nil {
		log.Printf("json.NewEncoder().Encode() failed: %v", err)
	}
}
