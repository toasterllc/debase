package DebaseLicenseServer

import (
	"encoding/json"

	"heytoaster.com/DebaseLicenseServer/license"
)

// Commands

const (
	LookupLicense    = "LookupLicense"
	LookupTrial      = "LookupTrial"
	SendLicenseEmail = "SendLicenseEmail"
)

type Command struct {
	Type    string          `json:"type"`
	Payload json.RawMessage `json:"payload"`
}

type LookupLicenseCommand struct {
	Email       string `json:"email"`
	LicenseCode string `json:"licenseCode"`
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

type LookupTrialCommand struct {
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

type SendLicenseEmailCommand struct {
	Email string `json:"email"`
}

// Replies

type Reply struct {
	Error   string      `json:"error"`
	Payload interface{} `json:"payload"`
}

type LookupLicenseReply license.HTTPSealedLicense
type LookupTrialReply license.HTTPSealedLicense
type SendEmailReply struct{}
