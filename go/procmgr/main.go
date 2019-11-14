package main

import (
	"flag"
	"fmt"
	"io"
	"os"
	"time"
)

type nullWriter struct {
}

var _ io.Writer = nullWriter{}

func (nullWriter) Write(p []byte) (n int, err error) {
	return len(p), nil
}

func main() {
	cli := flag.NewFlagSet(os.Args[0], flag.ContinueOnError)
	cli.Usage = doNothing
	cli.SetOutput(nullWriter{})

	basedir := cli.String("basedir", "", "PATH")

	if errPr := cli.Parse(os.Args[1:]); errPr != nil {
		logError(errPr.Error(), 2)
	}

	if *basedir == "" {
		logError("-basedir missing", 2)
	}

	if errML := newManager(*basedir).messagesLoop(); errML != nil {
		logError(errML.Error(), 1)
	}
}

func logError(err string, exit int) {
	fmt.Printf(
		"[%s] \x1b[31m\x1b[1mcritical\x1b[0m/Process: %s\n",
		time.Now().Format("2006-01-02 15:04:05 -0700"), err,
	)

	os.Exit(exit)
}

func doNothing() {
}
