package DebaseLicenseServer

import (
	"encoding/json"

	"heytoaster.com/DebaseLicenseServer/license"
)

// Commands

const (
	LicenseLookup    = "LicenseLookup"
	TrialLookup      = "TrialLookup"
	LicenseEmailSend = "LicenseEmailSend"
)

type Command struct {
	Type    string          `json:"type"`
	Payload json.RawMessage `json:"payload"`
}

type CommandLicenseLookup struct {
	Email       string `json:"email"`
	LicenseCode string `json:"licenseCode"`
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

type CommandTrialLookup struct {
	MachineId   string `json:"machineId"`
	MachineInfo string `json:"machineInfo"`
}

type CommandLicenseEmailSend struct {
	Email string `json:"email"`
}

// Replies

type Reply interface{}

type ReplyLicenseLookup struct {
	Error   string `json:"error"`
	Payload struct {
		License license.HTTPSealedLicense `json:"license"`
	} `json:"payload"`
}

type ReplyLicenseEmailSend struct{}
