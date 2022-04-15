package license

import (
	"crypto/sha512"
	"encoding/hex"
	"time"
)

// Database structs

type DBUserId string

const DBUserIdLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters

type DBMachine struct {
	MachineId   MachineId
	MachineInfo string
	Timestamp   int64
}

type DBTrialLicense struct {
	Version    uint32
	Expiration int64
	Machines   []Machines
}

type DBUserLicense struct {
	LicenseCode LicenseCode
	Machines    []Machines
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
