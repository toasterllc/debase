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
	MachineInfo MachineInfo
	Timestamp   int64
	IssueCount  int64
}

type DBTrial struct {
	Version    Version
	Expiration int64
	Machine    DBMachine
}

type DBLicense struct {
	Version   Version
	Timestamp int64
	Machines  map[MachineId]*DBMachine
	PaymentId string
}

type DBLicenses struct {
	Email                     string
	Licenses                  map[LicenseCode]*DBLicense
	LastLicenseEmailTimestamp int64
}

// Helper Functions

func DBUserIdForEmail(domain string, email Email) DBUserId {
	sha := sha512.New512_256()
	sha.Write([]byte(domain + ":" + string(email)))
	return DBUserId(hex.EncodeToString(sha.Sum(nil)))
}

func DBMachineCreate(minfo MachineInfo) *DBMachine {
	return &DBMachine{
		MachineInfo: minfo,
		Timestamp:   time.Now().Unix(),
		IssueCount:  1,
	}
}
