package main

import (
	"reflect"
	"runtime"
	"syscall"
	"time"
	"unsafe"
)

/*
#include "c.h"
// invokeCallback
// uchar

#include <stdint.h>
// uint64_t
// uintptr_t
*/
import "C"

//export NewManager
func NewManager() C.uintptr_t {
	return C.uintptr_t(uintptr(unsafe.Pointer(newManager())))
}

//export FreeManager
func FreeManager(mgr C.uintptr_t) {
	(*manager)(unsafe.Pointer(uintptr(mgr))).Close()
}

//export ManagerBeforeFork
func ManagerBeforeFork(mgr C.uintptr_t) C.int {
	errBF := (*manager)(unsafe.Pointer(uintptr(mgr))).beforeFork()
	switch e := errBF.(type) {
	case nil:
		return 0
	case syscall.Errno:
		return C.int(e)
	default:
		return -1
	}
}

//export ManagerAfterFork
func ManagerAfterFork(mgr C.uintptr_t, child C.uchar) {
	(*manager)(unsafe.Pointer(uintptr(mgr))).afterFork(child != 0)
}

//export ManagerReadLoop
func ManagerReadLoop(mgr C.uintptr_t) {
	(*manager)(unsafe.Pointer(uintptr(mgr))).readLoop()
}

//export ManagerWriteLoop
func ManagerWriteLoop(mgr C.uintptr_t) {
	(*manager)(unsafe.Pointer(uintptr(mgr))).writeLoop()
}

//export ManagerSpawn
func ManagerSpawn(mgr, args, extraEnv C.uintptr_t, timeout C.double, cb, cbData C.uintptr_t) C.uint64_t {
	return C.uint64_t((*manager)(unsafe.Pointer(uintptr(mgr))).spawn(
		copyCStrs(args), copyCStrs(extraEnv), time.Duration(float64(timeout)*float64(time.Second)),
		callback{uintptr(cb), uintptr(cbData)},
	))
}

func invokeCallback(cb callback, es *exitStatus) {
	if cb.function != 0 {
		defer runtime.KeepAlive(es)

		sh := *(*reflect.SliceHeader)(unsafe.Pointer(&es.output))
		C.invokeCallback(
			C.uintptr_t(cb.function), C.uintptr_t(cb.data), C.uint64_t(es.iPid),
			C.pid_t(es.pid), C.int(es.exitCode), C.uintptr_t(sh.Data), C.size_t(sh.Len),
			C.double(timeToFloat(es.execStart)), C.double(timeToFloat(es.execEnd)),
		)
	}
}

func copyCStrs(cStrs C.uintptr_t) []string {
	sh := reflect.SliceHeader{
		Data: uintptr(cStrs),
		Len:  maxInt,
		Cap:  maxInt,
	}
	sl := *(*[]unsafe.Pointer)(unsafe.Pointer(&sh))
	runtime.KeepAlive(&sh)

	var res []string
	for i := 0; sl[i] != nil; i++ {
		sh := reflect.SliceHeader{
			Data: uintptr(sl[i]),
			Len:  maxInt,
			Cap:  maxInt,
		}
		sl := *(*[]byte)(unsafe.Pointer(&sh))
		runtime.KeepAlive(&sh)

		i := 0
		for sl[i] != 0 {
			i++
		}

		res = append(res, string(sl[:i]))
	}

	return res
}

func timeToFloat(t time.Time) float64 {
	sec := t.Unix()
	return float64(sec) + float64(t.Sub(time.Unix(sec, 0)))/float64(time.Second)
}
