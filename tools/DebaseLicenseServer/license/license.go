package license

import (
	"crypto/sha512"
)

type UserId string
type LicenseCode string
type MachineId string

const UserIdLen = 2 * sha512.Size256    // 2* because 1 byte == 2 hex characters
const MachineIdLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters

type TrialLicense struct {
	MachineId  MachineId
	Version    uint32
	Expiration int64
}

type UserLicense struct {
	LicenseCode LicenseCode
	MachineIds  []MachineId
	Version     uint32
}

type UserLicenses struct {
	Email    string
	Licenses []UserLicense
}
