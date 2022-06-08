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

var DebaseProductId = C.GoString(C.DebaseProductId)
var DebaseKeyPrivate = *(*[ed25519.SeedSize]byte)(unsafe.Pointer(&C.DebaseKeyPrivate))
var DebaseVersion = uint32(C.DebaseVersion)

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

func licensesCollection() string {
	switch Environment {
	case EnvironmentProd:
		return "debase-licenses"
	case EnvironmentStage:
		return "debase-licenses-stage"
	default:
		panic("unknown Environment")
	}
}

func trialsCollection() string {
	switch Environment {
	case EnvironmentProd:
		return "debase-trials"
	case EnvironmentStage:
		return "debase-trials-stage"
	default:
		panic("unknown Environment")
	}
}
