package main

import (
	"DebaseLicenseServer/license"
	"context"
	"crypto/ed25519"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"regexp"
	"strings"
	"time"
	"unsafe"

	"github.com/aws/aws-lambda-go/lambda"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/dynamodb"
	"github.com/aws/aws-sdk-go/service/dynamodb/dynamodbattribute"
	"github.com/aws/aws-sdk-go/service/dynamodb/dynamodbiface"
)

// #cgo CFLAGS: -DDebaseLicenseServer=1
// #include "../../src/Debase.h"
import "C"

const LicenseTableName = "License"
const TrialTableName = "Trial"
const TrialDuration = 7 * 24 * time.Hour

var db dynamodbiface.DynamoDBAPI

func hashSanitize(hash string) (string, error) {
	// Validate the machine id, which is always required
	if len(hash) != license.HashStringLen {
		return "", fmt.Errorf("invalid length")
	}

	hash = strings.ToLower(hash)

	validHash := regexp.MustCompile(`^[a-f0-9]+$`)
	if !validHash.MatchString(hash) {
		return "", fmt.Errorf("invalid characters")
	}

	return hash, nil
}

func machineIdSanitize(mid license.MachineId) (license.MachineId, error) {
	r, err := hashSanitize(string(mid))
	return license.MachineId(r), err
}

func userIdSanitize(uid license.UserId) (license.UserId, error) {
	r, err := hashSanitize(string(uid))
	return license.UserId(r), err
}

func registerCodeSanitize(code license.RegisterCode) (license.RegisterCode, error) {
	regex := regexp.MustCompile(`^[a-z0-9]+$`)
	if !regex.MatchString(string(code)) {
		return "", fmt.Errorf("invalid characters")
	}
	return code, nil
}

func reqSanitize(req license.Request) (license.Request, error) {
	var err error
	req.MachineId, err = machineIdSanitize(req.MachineId)
	if err != nil {
		return license.Request{}, fmt.Errorf("invalid machine id (%v): %w", req.MachineId, err)
	}

	if req.UserId != "" {
		req.UserId, err = userIdSanitize(req.UserId)
		if err != nil {
			return license.Request{}, fmt.Errorf("invalid user id (%v): %w", req.UserId, err)
		}
	}

	req.RegisterCode, err = registerCodeSanitize(req.RegisterCode)
	if err != nil {
		return license.Request{}, fmt.Errorf("invalid register code (%v): %w", req.UserId, err)
	}

	return req, nil

	// // Validate the machine id, which is always required
	// if len(req.MachineId) != MachineIdLen {
	//     return license.Request{}, fmt.Errorf("invalid request")
	// }
	//
	//     req.MachineId = strings.ToLower(req.MachineId)
	//     validHex := regexp.MustCompile(`^[a-f0-9]+$`)
	//     if !validHex.MatchString(req.MachineId) {
	//         return license.Request{}, fmt.Errorf("invalid request")
	//     }
	//
	// // Required
	// MachineId MachineId `json:"machineId"`
	// // Optional
	// UserId       UserId       `json:"userId"`
	// RegisterCode RegisterCode `json:"registerCode"`
}

func trialLicenseCreate(mid license.MachineId) license.License {
	expiration := time.Now().Add(TrialDuration)
	return license.License{
		MachineId:  mid,
		Version:    uint32(C.DebaseVersion),
		Expiration: expiration.Unix(),
	}
}

func HandlerLicense(ctx context.Context, req license.Request) (license.Response, error) {
	return license.Response{}, nil
}

type SealedLicenseRecord struct {
	license.MachineId     `json:"machineId"`
	license.SealedLicense `json:"license"`
}

func HandlerTrial(ctx context.Context, mid license.MachineId) (license.Response, error) {
	// Create trial, convert to json
	lic := trialLicenseCreate(mid)
	payloadb, err := json.Marshal(lic)
	if err != nil {
		return license.Response{}, fmt.Errorf("json.Marshal failed: %w", err)
	}
	payload := string(payloadb)

	//     //
	// h := sha512.New512_256()
	// h.Write(licb)
	// sigb := h.Sum(nil)

	key := ed25519.PrivateKey(*(*[]byte)(unsafe.Pointer(&C.DebaseKeyPrivate)))
	sigb := ed25519.Sign(key, payloadb)
	sig := hex.EncodeToString(sigb)
	sealed := license.SealedLicense{
		Payload:   payload,
		Signature: sig,
	}

	av, err := dynamodbattribute.MarshalMap(SealedLicenseRecord{
		MachineId:     mid,
		SealedLicense: sealed,
	})
	if err != nil {
		return license.Response{}, fmt.Errorf("dynamodbattribute.MarshalMap failed: %w", err)
	}

	_, err = db.PutItemWithContext(ctx, &dynamodb.PutItemInput{
		Item:                av,
		TableName:           aws.String(TrialTableName),
		ConditionExpression: aws.String("attribute_not_exists(machineId)"),
	})

	if err != nil {
		return license.Response{}, fmt.Errorf("db.PutItem failed: %w", err)
	}

	return license.Response{
		License: sealed,
	}, nil
}

func Handler(ctx context.Context, req license.Request) (license.Response, error) {
	req, err := reqSanitize(req)
	if err != nil {
		log.Printf("invalid request: %w", err)
		return license.Response{}, fmt.Errorf("invalid request")
	}

	if req.UserId != "" {
		// Register request
		resp, err := HandlerLicense(ctx, req)
		if err != nil {
			log.Printf("HandlerLicense failed: %w", err)
			return license.Response{}, fmt.Errorf("HandlerLicense failed")
		}
		return resp, nil

	} else {
		// Trial request
		resp, err := HandlerTrial(ctx, req.MachineId)
		if err != nil {
			log.Printf("HandlerTrial failed: %w", err)
			return license.Response{}, fmt.Errorf("HandlerTrial failed")
		}
		return resp, nil

		// return HandlerTrial(ctx, req.MachineId)
		//
		// // Trial request
		// sealed, err := trialCreate(req.MachineId)
		// if err != nil {
		//     log.Printf("trialCreate failed: %w", err)
		//     return license.Response{}, fmt.Errorf("trialCreate failed")
		// }

	}
}

func main() {

	aws, err := session.NewSession(&aws.Config{
		Region: aws.String(os.Getenv("AWS_REGION")),
	})

	if err != nil {
		log.Fatalf("session.NewSession failed: %v", err)
	}

	db = dynamodb.New(aws)
	lambda.Start(Handler)
}
