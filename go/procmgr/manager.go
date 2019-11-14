package main

import (
	"bufio"
	"bytes"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"sync"
	"syscall"
	"time"
)

type manager struct {
	ourEnv struct {
		vars []string
		once sync.Once
	}
	basedir string
}

func (m *manager) messagesLoop() error {
	every1s := time.NewTicker(time.Second)
	defer every1s.Stop()

	dir := path.Join(m.basedir, "spawn")
	chErrWR := make(chan error, 1)

	for {
		select {
		case errWR := <-chErrWR:
			return errWR
		case <-every1s.C:
			entries, errRD := ioutil.ReadDir(dir)
			if errRD != nil {
				return errRD
			}

			chErrRR := make(chan error, len(entries))

			for _, entry := range entries {
				go m.handleRequest(entry, chErrRR, chErrWR)
			}

			for range entries {
				if errRR := <-chErrRR; errRR != nil {
					return errRR
				}
			}
		}
	}
}

func (m *manager) handleRequest(file os.FileInfo, chErrRR chan<- error, chErrWR chan<- error) {
	var req spawnRequest
	if errRR := m.readRequest(file, &req); errRR != nil {
		chErrRR <- errRR
		return
	}

	chErrRR <- nil

	m.ourEnv.once.Do(m.initEnv)

	var prog string
	var args []string

	if len(req.args) > 0 {
		prog = req.args[0]
		args = req.args[1:]
	}

	cmd := exec.Command(prog, args...)
	var out bytes.Buffer

	cmd.Env = append(m.ourEnv.vars, req.extraEnv...)
	cmd.Stdout = &out
	cmd.Stderr = &out
	cmd.SysProcAttr = &syscall.SysProcAttr{Setsid: true}

	if errSt := cmd.Start(); errSt == nil {
		start := time.Now()

		timer := time.NewTimer(req.timeout)
		defer timer.Stop()

		waitErr := make(chan error, 1)
		go waitForCmd(cmd, waitErr)

		var errWt error
		var end time.Time

		select {
		case errWt = <-waitErr:
			end = time.Now()
		case <-timer.C:
			syscall.Kill(-cmd.Process.Pid, syscall.SIGKILL)

			errWt = <-waitErr
			end = time.Now()

			out.Write([]byte("<Timeout exceeded.>"))
		}

		if errWt == nil {
			errWR := m.writeResponse(&exitStatus{req.iPid, cmd.Process.Pid, 0, out.Bytes(), start, end}, file)
			if errWR != nil {
				select {
				case chErrWR <- errWR:
				default:
				}
			}
		} else if ee, ok := errWt.(*exec.ExitError); ok {
			var exitCode int
			status := ee.ProcessState.Sys().(syscall.WaitStatus)

			if status.Exited() {
				exitCode = status.ExitStatus()
			} else if status.Signaled() {
				exitCode = 128 + int(status.Signal())
			} else if status.Stopped() {
				exitCode = 128 + int(status.StopSignal())
			} else if status.Continued() {
				exitCode = 128 + int(syscall.SIGCONT)
			} else {
				exitCode = 128

				out.Reset()
				out.Write([]byte(ee.Error()))
			}

			errWR := m.writeResponse(&exitStatus{req.iPid, cmd.Process.Pid, exitCode, out.Bytes(), start, end}, file)
			if errWR != nil {
				select {
				case chErrWR <- errWR:
				default:
				}
			}
		} else {
			errWR := m.writeResponse(&exitStatus{req.iPid, -1, 128, []byte(errWt.Error()), start, end}, file)
			if errWR != nil {
				select {
				case chErrWR <- errWR:
				default:
				}
			}
		}
	} else {
		now := time.Now()
		errWR := m.writeResponse(&exitStatus{req.iPid, -1, 128, []byte(errSt.Error()), now, now}, file)
		if errWR != nil {
			select {
			case chErrWR <- errWR:
			default:
			}
		}
	}
}

func (m *manager) readRequest(in os.FileInfo, out *spawnRequest) error {
	fullPath := path.Join(m.basedir, "spawn", in.Name())

	f, errOp := os.Open(fullPath)
	if errOp != nil {
		return errOp
	}
	defer f.Close()

	if _, errRF := out.ReadFrom(bufio.NewReader(f)); errRF != nil {
		return errRF
	}

	return os.Remove(fullPath)
}

func (m *manager) initEnv() {
	m.ourEnv.vars = os.Environ()
}

func waitForCmd(cmd *exec.Cmd, out chan<- error) {
	out <- cmd.Wait()
}

func (m *manager) writeResponse(in *exitStatus, out os.FileInfo) error {
	tmp := path.Join(m.basedir, "tmp", out.Name())

	f, errCr := os.Create(tmp)
	if errCr != nil {
		return errCr
	}
	defer f.Close()

	{
		buf := bufio.NewWriter(f)

		if _, errWT := in.WriteTo(buf); errWT != nil {
			return errWT
		}

		if errFl := buf.Flush(); errFl != nil {
			return errFl
		}
	}

	return os.Rename(tmp, path.Join(m.basedir, "exit", out.Name()))
}

func newManager(basedir string) *manager {
	return &manager{basedir: basedir}
}
