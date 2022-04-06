package DebaseLicenseServer

import (
	"fmt"
	"net/http"
)

func DebaseLicenseServer(w http.ResponseWriter, r *http.Request) {
	fmt.Fprint(w, "hello world meow")
}
