package main

import "io"

type completeReader struct {
	reader io.Reader
}

var _ io.Reader = completeReader{}

func (cr completeReader) Read(p []byte) (n int, err error) {
	for len(p) > 0 {
		m, errRd := cr.reader.Read(p)
		n += m

		if errRd != nil {
			err = errRd
			break
		}

		p = p[m:]
	}

	return
}
