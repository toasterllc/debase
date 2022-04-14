package license

import (
	"crypto/sha512"
	"fmt"
	"net/mail"
	"regexp"
	"strings"
)

// License types used by both database and HTTP

type Email string
type LicenseCode string
type MachineId string

const MachineIdLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters

func hashStringSanitize(str string) (string, error) {
	str = strings.ToLower(str)

	validHash := regexp.MustCompile(`^[a-f0-9]+$`)
	if !validHash.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}

	return str, nil
}

func EmailForString(str string) (Email, error) {
	parsed, err := mail.ParseAddress(str)
	if err != nil {
		return "", fmt.Errorf("mail.ParseAddress failed: %w", err)
	}

	return Email(parsed.Address), nil
}

func MachineIdForString(str string) (MachineId, error) {
	if len(str) != MachineIdLen {
		return "", fmt.Errorf("invalid length")
	}

	str, err := hashStringSanitize(str)
	return MachineId(str), err
}

// func userIdForEmail(email Email) (UserId, error) {
//     if len(uid) != UserIdLen {
//         return "", fmt.Errorf("invalid length")
//     }
//
//     r, err := hashStringSanitize(string(uid))
//     return UserId(r), err
// }

func LicenseCodeForString(str string) (LicenseCode, error) {
	str = strings.ToLower(str)
	regex := regexp.MustCompile(`^[a-z0-9]+$`)
	if !regex.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}
	return LicenseCode(str), nil
}
