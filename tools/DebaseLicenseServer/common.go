package DebaseLicenseServer

import (
	"crypto/ed25519"
	"encoding/hex"
	"encoding/json"
	"errors"
	"unsafe"

	"heytoaster.com/DebaseLicenseServer/license"

	"cloud.google.com/go/firestore"
)

// #cgo CFLAGS: -DDebaseLicenseServer=1
// #include "Debase.h"
import "C"

type Reply interface{}

const LicensesCollection = "debase-licenses"
const TrialsCollection = "debase-trials"

var DebaseProductId = C.GoString(C.DebaseProductId)
var DebaseKeyPrivate = *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
var DebaseVersion = uint32(C.DebaseVersion)

var LicenseNotFoundErr = errors.New("No matching license was found.")
var MachineLimitErr = errors.New("The maximum number of machines has already been registered for this license code.")
var TrialExpiredErr = errors.New("The existing trial has already expired.")
var UnknownErr = errors.New("An unknown error occurred.")

var Db *firestore.Client

func sealedLicense(lic license.HTTPLicense) license.HTTPSealedLicense {
	// Convert `lic` to json
	payload, err := json.Marshal(lic)
	if err != nil {
		panic("json.Marshal failed on license.HTTPLicense")
	}

	key := ed25519.NewKeyFromSeed(DebaseKeyPrivate[:])
	sig := ed25519.Sign(key, payload)
	return license.HTTPSealedLicense{
		Payload:   string(payload),
		Signature: hex.EncodeToString(sig),
	}
}
