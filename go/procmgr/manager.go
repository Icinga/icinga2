package main

import (
	"bufio"
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"sync"
	"syscall"
	"time"
)

type manager struct {
	writeQueue struct {
		items []*exitStatus
		lock  sync.Mutex
		feed  chan struct{}
	}
	ourEnv struct {
		vars []string
		once sync.Once
	}
}

func (m *manager) messagesLoop() {
	var wg sync.WaitGroup
	wg.Add(2)

	readLoopDone := make(chan struct{})

	go m.readLoop(&wg, readLoopDone)
	go m.writeLoop(&wg, readLoopDone)

	wg.Wait()
}

func (m *manager) readLoop(wg *sync.WaitGroup, readLoopDone chan<- struct{}) {
	defer wg.Done()
	defer close(readLoopDone)

	buf := bufio.NewReader(os.Stdin)

	for {
		msg := &spawnRequest{}
		if _, errRF := msg.ReadFrom(buf); errRF != nil {
			fmt.Fprintln(os.Stderr, errRF.Error())
			break
		}

		go m.handleRequest(msg)
	}
}

func (m *manager) writeLoop(wg *sync.WaitGroup, readLoopDone <-chan struct{}) {
	defer wg.Done()

	buf := bufio.NewWriter(os.Stdout)

DequeueLoop:
	for {
		select {
		case <-readLoopDone:
			break DequeueLoop
		case <-m.writeQueue.feed:
		}

		m.writeQueue.lock.Lock()
		queue := m.writeQueue.items
		m.writeQueue.items = nil
		m.writeQueue.lock.Unlock()

		for range queue[1:] {
			<-m.writeQueue.feed
		}

		for _, msg := range queue {
			if _, errWT := msg.WriteTo(buf); errWT != nil {
				fmt.Fprintln(os.Stderr, errWT.Error())
				break DequeueLoop
			}
		}

		if buf.Flush() != nil {
			break
		}
	}
}

func (m *manager) enqueueMessage(msg *exitStatus) {
	m.writeQueue.lock.Lock()
	defer m.writeQueue.lock.Unlock()

	m.writeQueue.items = append(m.writeQueue.items, msg)
	m.writeQueue.feed <- struct{}{}
}

func (m *manager) handleRequest(req *spawnRequest) {
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
			m.enqueueMessage(&exitStatus{req.iPid, cmd.Process.Pid, 0, out.Bytes(), start, end})
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

			m.enqueueMessage(&exitStatus{req.iPid, cmd.Process.Pid, exitCode, out.Bytes(), start, end})
		} else {
			m.enqueueMessage(&exitStatus{req.iPid, -1, 128, []byte(errWt.Error()), start, end})
		}
	} else {
		now := time.Now()
		m.enqueueMessage(&exitStatus{req.iPid, -1, 128, []byte(errSt.Error()), now, now})
	}
}

func (m *manager) initEnv() {
	m.ourEnv.vars = os.Environ()
}

func waitForCmd(cmd *exec.Cmd, out chan<- error) {
	out <- cmd.Wait()
}

func newManager() *manager {
	mgr := &manager{}
	mgr.writeQueue.feed = make(chan struct{}, int(^uint(0)>>1))

	return mgr
}
