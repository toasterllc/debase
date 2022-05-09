package license

// HTTP Structs

type HTTPLicense struct {
	Email       string `json:"email"`
	LicenseCode string `json:"licenseCode"`
	MachineId   string `json:"machineId"`
	Version     uint32 `json:"version"`
	Expiration  int64  `json:"expiration"`
}

type HTTPSealedLicense struct {
	Payload   string `json:"payload"`
	Signature string `json:"signature"`
}
