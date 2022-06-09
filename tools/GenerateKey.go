package main

import (
	"crypto/ed25519"
	"crypto/rand"
	"fmt"
)

func main() {
	pub, priv, err := ed25519.GenerateKey(rand.Reader)
	if err != nil {
		panic(err.Error())
	}

	fmt.Printf("Public key:\n")
	for _, b := range pub {
		fmt.Printf("0x%02x, ", b)
	}
	fmt.Printf("\n\n")

	fmt.Printf("Private key:\n")
	for _, b := range priv.Seed() {
		fmt.Printf("0x%02x, ", b)
	}
	fmt.Printf("\n\n")
}
