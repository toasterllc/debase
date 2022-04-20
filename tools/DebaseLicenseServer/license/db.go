package license

// Database structs

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
