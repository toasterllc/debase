package license

import (
	"crypto/sha512"
	"encoding/hex"
)

// Database structs

type DBUserId string

const DBUserIdLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters

type DBTrialLicense struct {
	Version    uint32
	Expiration int64
}

type DBUserLicense struct {
	LicenseCode LicenseCode
	MachineIds  []MachineId
	Version     uint32
}

type DBUserLicenses struct {
	Email    string
	Licenses []DBUserLicense
}

func DBUserIdForEmail(domain string, email Email) DBUserId {
	sha := sha512.New512_256()
	sha.Write([]byte(domain + ":" + string(email)))
	return DBUserId(hex.EncodeToString(sha.Sum(nil)))
}
