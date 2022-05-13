package DebaseLicenseServer

import (
	"heytoaster.com/DebaseLicenseServer/license"
)

// Commands

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

type ReplyLicenseLookup struct {
	Error   string                    `json:"error"`
	License license.HTTPSealedLicense `json:"license"`
}

type ReplyLicenseEmailSend struct{}
