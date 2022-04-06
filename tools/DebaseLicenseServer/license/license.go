package license

import (
	"crypto/sha512"
)

type UserId string
type RegisterCode string
type MachineId string

const HashStringLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters
// const UserIdLen = 2 * crypto.Size256     // 2* because 1 byte == 2 hex characters
// const MachineIdLen = 2 * crypto.Size256  // 2* because 1 byte == 2 hex characters

type License struct {
	UserId       UserId       `json:"userId"`
	RegisterCode RegisterCode `json:"registerCode"`
	MachineId    MachineId    `json:"machineId"`
	Version      uint32       `json:"version"`
	Expiration   int64        `json:"expiration"`
}

type SealedLicense struct {
	Payload   string `json:"payload"`
	Signature string `json:"signature"`
}

type Request struct {
	// Required
	MachineId MachineId `json:"machineId"`
	// Optional (present=register request, absent=trial request)
	UserId       UserId       `json:"userId"`
	RegisterCode RegisterCode `json:"registerCode"`
}

type Response struct {
	Error   string        `json:"error"`
	License SealedLicense `json:"license"`
}
