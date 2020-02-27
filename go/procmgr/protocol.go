package main

import (
	"io"
	"runtime"
	"time"
	"unsafe"
)

type spawnRequest struct {
	iPid     uint64
	args     []string
	extraEnv []string
	timeout  time.Duration
}

var _ io.ReaderFrom = (*spawnRequest)(nil)

func (sr *spawnRequest) ReadFrom(r io.Reader) (n int64, err error) {
	var rsr rawSpawnRequest

	{
		m, e := rsr.ReadFrom(r)
		n += m

		if e != nil {
			err = e
			return
		}
	}

	sr.iPid = rsr.iPid
	sr.timeout = time.Duration(rsr.timeout * float64(time.Second))

	rbaMap := [2]struct {
		from []rawBytesArray32
		to   *[]string
	}{
		{make([]rawBytesArray32, int(rsr.args)), &sr.args},
		{make([]rawBytesArray32, int(rsr.extraEnv)), &sr.extraEnv},
	}

	for _, rbas := range rbaMap {
		*rbas.to = make([]string, 0, len(rbas.from))
	}

	for _, rbas := range rbaMap {
		for i := range rbas.from {
			m, e := rbas.from[i].ReadFrom(r)
			n += m

			if e != nil {
				err = e
				return
			}
		}
	}

	for _, rbas := range rbaMap {
		for _, rba := range rbas.from {
			buf := make([]byte, int(rba.length))

			m, e := completeReader{r}.Read(buf)
			n += int64(m)

			if e != nil {
				err = e
				return
			}

			*rbas.to = append(*rbas.to, string(buf))
		}
	}

	return
}

type exitStatus struct {
	iPid      uint64
	pid       int
	exitCode  int
	output    []byte
	execStart time.Time
	execEnd   time.Time
}

var _ io.WriterTo = (*exitStatus)(nil)

func (es *exitStatus) WriteTo(w io.Writer) (n int64, err error) {
	res := rawExitStatus{
		es.iPid,
		int32(es.pid),
		int32(es.exitCode),
		uint64(len(es.output)),
		timeToFloat(es.execStart),
		timeToFloat(es.execEnd),
	}

	{
		m, e := res.WriteTo(w)
		n += m

		if e != nil {
			err = e
			return
		}
	}

	m, e := w.Write(es.output)
	n += int64(m)

	err = e
	return
}

type rawSpawnRequest struct {
	iPid     uint64
	args     uint32
	extraEnv uint32
	timeout  float64
}

var _ io.ReaderFrom = (*rawSpawnRequest)(nil)

func (sr *rawSpawnRequest) ReadFrom(r io.Reader) (n int64, err error) {
	m, e := completeReader{r}.Read((*(*[unsafe.Sizeof(*sr)]byte)(unsafe.Pointer(sr)))[:])
	runtime.KeepAlive(sr)

	return int64(m), e
}

type rawExitStatus struct {
	iPid      uint64
	pid       int32
	exitCode  int32
	output    uint64
	execStart float64
	execEnd   float64
}

var _ io.WriterTo = (*rawExitStatus)(nil)

func (es *rawExitStatus) WriteTo(w io.Writer) (n int64, err error) {
	m, e := w.Write((*(*[unsafe.Sizeof(*es)]byte)(unsafe.Pointer(es)))[:])
	runtime.KeepAlive(es)

	return int64(m), e
}

type rawBytesArray32 struct {
	length uint32
}

var _ io.ReaderFrom = (*rawBytesArray32)(nil)

func (ba *rawBytesArray32) ReadFrom(r io.Reader) (n int64, err error) {
	m, e := completeReader{r}.Read((*(*[unsafe.Sizeof(*ba)]byte)(unsafe.Pointer(ba)))[:])
	runtime.KeepAlive(ba)

	return int64(m), e
}

func timeToFloat(t time.Time) float64 {
	sec := t.Unix()
	return float64(sec) + float64(t.Sub(time.Unix(sec, 0)))/float64(time.Second)
}
