package main

import (
	"bufio"
	"bytes"
	"encoding/gob"
	"io"
	"net"
	"os"
	"os/exec"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
)

type callback struct {
	function, data uintptr
}

type manager struct {
	ipcConns struct {
		parent, child net.Conn
	}
	writeQueue struct {
		items []interface{}
		lock  sync.Mutex
		feed  chan struct{}
	}
	ourEnv struct {
		vars []string
		once sync.Once
	}
	pendingCallbacks struct {
		items map[uint64]callback
		lock  sync.Mutex
	}
	nextIPid     uint64
	readLoopDone chan struct{}
}

var _ io.Closer = (*manager)(nil)

type spawnRequest struct {
	iPid     uint64
	args     []string
	extraEnv []string
	timeout  time.Duration
}

var _ = func() struct{} {
	gob.Register((*spawnRequest)(nil))
	return struct{}{}
}()

type exitStatus struct {
	iPid      uint64
	pid       int
	exitCode  int
	output    []byte
	execStart time.Time
	execEnd   time.Time
}

var _ = func() struct{} {
	gob.Register((*exitStatus)(nil))
	return struct{}{}
}()

var aliveManagers = struct {
	items map[*manager]struct{}
	lock  sync.Mutex
}{}

func (m *manager) Close() (err error) {
	for _, conn := range [2]net.Conn{m.ipcConns.parent, m.ipcConns.child} {
		if conn != nil {
			if errCl := conn.Close(); errCl != nil && err == nil {
				err = errCl
			}
		}
	}

	aliveManagers.lock.Lock()
	defer aliveManagers.lock.Unlock()

	delete(aliveManagers.items, m)

	return
}

func (m *manager) beforeFork() error {
	fds, errSP := socketpair(syscall.AF_UNIX, syscall.SOCK_STREAM, 0)
	if errSP != nil {
		return errSP
	}

	defer closeFdIfSet(&fds[0])
	defer closeFdIfSet(&fds[1])

	syscall.SetNonblock(fds[0], true)
	syscall.SetNonblock(fds[1], true)

	f0 := os.NewFile(uintptr(fds[0]), "")
	defer f0.Close()
	fds[0] = -1

	f1 := os.NewFile(uintptr(fds[1]), "")
	defer f1.Close()
	fds[1] = -1

	c0, errFC0 := net.FileConn(f0)
	if errFC0 != nil {
		return errFC0
	}

	defer closeConnIfSet(&c0)

	c1, errFC1 := net.FileConn(f1)
	if errFC1 != nil {
		return errFC1
	}

	defer closeConnIfSet(&c1)

	m.ipcConns.parent = c0
	c0 = nil

	m.ipcConns.child = c1
	c1 = nil

	return nil
}

func (m *manager) afterFork(child bool) {
	if child {
		m.ipcConns.parent.Close()
	} else {
		m.ipcConns.child.Close()
	}
}

func (m *manager) spawn(args, extraEnv []string, timeout time.Duration, cb callback) uint64 {
	iPid := atomic.AddUint64(&m.nextIPid, 1) - 1
	m.registerCallback(iPid, cb)
	m.enqueueMessage(&spawnRequest{iPid, args, extraEnv, timeout})

	return iPid
}

func (m *manager) registerCallback(iPid uint64, cb callback) {
	m.pendingCallbacks.lock.Lock()
	defer m.pendingCallbacks.lock.Unlock()

	m.pendingCallbacks.items[iPid] = cb
}

func (m *manager) popCallback(iPid uint64) callback {
	m.pendingCallbacks.lock.Lock()
	defer m.pendingCallbacks.lock.Unlock()

	cb := m.pendingCallbacks.items[iPid]
	delete(m.pendingCallbacks.items, iPid)

	return cb
}

func (m *manager) readLoop() {
	defer close(m.readLoopDone)

	dec := gob.NewDecoder(bufio.NewReader(m.myConn()))
	var msg *interface{}

	for dec.Decode(&msg) == nil {
		go m.handleRequest(*msg)
	}
}

func (m *manager) writeLoop() {
	buf := bufio.NewWriter(m.myConn())
	dec := gob.NewEncoder(buf)

DequeueLoop:
	for {
		select {
		case <-m.readLoopDone:
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
			if dec.Encode(&msg) != nil {
				break DequeueLoop
			}
		}

		if buf.Flush() != nil {
			break
		}
	}
}

func (m *manager) myConn() net.Conn {
	if m.ipcConns.parent == nil {
		return m.ipcConns.child
	} else {
		return m.ipcConns.parent
	}
}

func (m *manager) enqueueMessage(msg interface{}) {
	m.writeQueue.lock.Lock()
	defer m.writeQueue.lock.Unlock()

	m.writeQueue.items = append(m.writeQueue.items, msg)
	m.writeQueue.feed <- struct{}{}
}

func (m *manager) handleRequest(req interface{}) {
	switch req := req.(type) {
	case *spawnRequest:
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
		cmd.SysProcAttr.Setsid = true

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
	case *exitStatus:
		invokeCallback(m.popCallback(req.iPid), req)
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
	mgr.writeQueue.feed = make(chan struct{}, maxInt)
	mgr.pendingCallbacks.items = map[uint64]callback{}
	mgr.readLoopDone = make(chan struct{})

	aliveManagers.lock.Lock()
	defer aliveManagers.lock.Unlock()

	if aliveManagers.items == nil {
		aliveManagers.items = map[*manager]struct{}{}
	}

	aliveManagers.items[mgr] = struct{}{}

	return mgr
}

func socketpair(domain, typ, proto int) (fd [2]int, err error) {
	syscall.ForkLock.RLock()
	defer syscall.ForkLock.RUnlock()

	fd, err = syscall.Socketpair(domain, typ, proto)

	if err == nil {
		syscall.CloseOnExec(fd[0])
		syscall.CloseOnExec(fd[1])
	}

	return
}

func closeFdIfSet(fd *int) error {
	return syscall.Close(*fd)
}

func closeConnIfSet(conn *net.Conn) error {
	if *conn == nil {
		return nil
	} else {
		return (*conn).Close()
	}
}
