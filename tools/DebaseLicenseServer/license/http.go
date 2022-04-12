package license

// HTTP Structs

type HTTPTrialLicense struct {
	MachineId  MachineId `json:"machineId"`
	Version    uint32    `json:"version"`
	Expiration int64     `json:"expiration"`
}

type HTTPUserLicense struct {
	UserId      UserId      `json:"userId"`
	LicenseCode LicenseCode `json:"licenseCode"`
	MachineId   MachineId   `json:"machineId"`
	Version     uint32      `json:"version"`
}

type HTTPLicense interface {
	HTTPTrialLicense | HTTPUserLicense
}

type HTTPSealedLicense struct {
	Payload   string `json:"payload"`
	Signature string `json:"signature"`
}

type HTTPRequest struct {
	// Required
	MachineId MachineId `json:"machineId"`
	// Optional (present=license request, absent=trial request)
	UserId      UserId      `json:"userId"`
	LicenseCode LicenseCode `json:"licenseCode"`
}

type HTTPResponse struct {
	Error   string            `json:"error"`
	License HTTPSealedLicense `json:"license"`
}
