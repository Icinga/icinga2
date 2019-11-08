package main

import (
	"bytes"
	"reflect"
	"testing"
	"time"
)

func TestSpawnRequest_ReadFrom(t *testing.T) {
	in := []byte{
		23, 0, 0, 0, 0, 0, 0, 0,
		2, 0, 0, 0,
		3, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0x45, 0x40,
		4, 0, 0, 0,
		5, 0, 0, 0,
		6, 0, 0, 0,
		7, 0, 0, 0,
		8, 0, 0, 0,
		'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o',
		'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
	}

	out := spawnRequest{23, []string{"abcd", "efghi"}, []string{"jklmno", "pqrstuv", "wxyz0123"}, 42 * time.Second}

	var buf bytes.Buffer
	buf.Write(in)

	var actual spawnRequest

	if n, err := actual.ReadFrom(&buf); err != nil {
		t.Errorf(
			"var buf bytes.Buffer\nbuf.Write(%#v)\nvar out spawnRequest\n_, err := out.ReadFrom(&buf)\nerr != nil",
			in,
		)
	} else if n != int64(len(in)) {
		t.Errorf(
			"var buf bytes.Buffer\nbuf.Write(%#v)\nvar out spawnRequest\nn, _ := out.ReadFrom(&buf)\nn != %#v",
			in, len(in),
		)
	} else if !reflect.DeepEqual(actual, out) {
		t.Errorf(
			"var buf bytes.Buffer\nbuf.Write(%#v)\nvar out spawnRequest\nout.ReadFrom(&buf)\n!reflect.DeepEqual(%#v, out)",
			in, actual,
		)
	} else if buf.Len() != 0 {
		t.Errorf(
			"var buf bytes.Buffer\nbuf.Write(%#v)\nvar out spawnRequest\nout.ReadFrom(&buf)\nbuf.Len() != 0",
			in,
		)
	}
}

func TestExitStatus_WriteTo(t *testing.T) {
	in := exitStatus{23, 42, 777, []byte("abc"), time.Unix(1234567890, 500000000), time.Unix(1234567891, 250000000)}

	out := []byte{
		23, 0, 0, 0, 0, 0, 0, 0,
		42, 0, 0, 0,
		9, 3, 0, 0,
		3, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0xa0, 0xb4, 0x80, 0x65, 0xd2, 0x41,
		0, 0, 0xd0, 0xb4, 0x80, 0x65, 0xd2, 0x41,
		'a', 'b', 'c',
	}

	var buf bytes.Buffer

	if n, err := in.WriteTo(&buf); err != nil {
		t.Errorf("_, err := (%#v).WriteTo(&bytes.Buffer{}); err != nil", &in)
	} else if n != int64(len(out)) {
		t.Errorf("n, _ := (%#v).WriteTo(&bytes.Buffer{}); n != %#v", in, len(out))
	} else if !reflect.DeepEqual(buf.Bytes(), out) {
		t.Errorf("var buf bytes.Buffer\n(%#v).WriteTo(&buf)\n!reflect.DeepEqual(buf.Bytes(), %#v)", in, out)
	}
}
