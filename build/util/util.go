package util

import (
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

var (
	ProjectDir = func() string {
		_, filename, _, ok := runtime.Caller(0)
		Assert(ok)
		abs, err := filepath.Abs(filename)
		CheckErr(err)

		return filepath.Dir(filepath.Dir(filepath.Dir(abs)))
	}()

	UserDir     = filepath.Join(ProjectDir, "user")
	LibDir      = filepath.Join(ProjectDir, "lib")
	KernelDir   = filepath.Join(ProjectDir, "kern")
	IncludeDir  = filepath.Join(ProjectDir, "include")
	BuildDir    = filepath.Join(ProjectDir, "build")
	DotBuildDir = filepath.Join(ProjectDir, ".build")

	DepsDir  = filepath.Join(DotBuildDir, "deps")  // dep files
	CacheDir = filepath.Join(DotBuildDir, "cache") // cache flag files
	ObjDir   = filepath.Join(DotBuildDir, "obj")   // intermediate files
	OutDir   = filepath.Join(DotBuildDir, "out")   // output files
)

func EscapeSourcePath(targetDir, filePath string, extras ...string) string {
	Assert(filepath.IsAbs(targetDir))

	abs, err := filepath.Abs(filePath)
	CheckErr(err)
	Assert(!strings.HasPrefix(abs, targetDir))
	Assert(strings.HasPrefix(abs, ProjectDir))

	rel, err := filepath.Rel(ProjectDir, abs)
	CheckErr(err)

	pathSep := string(os.PathSeparator)
	cachePathName := strings.Join(append([]string{"C" + rel}, extras...), pathSep)
	cachePathName = strings.Replace(cachePathName, pathSep, ".", -1)

	return filepath.Join(targetDir, cachePathName)
}

func RunCmd(binary string, args []string) {
	cmd := exec.Command(binary, args...)

	stdout, err := cmd.StdoutPipe()
	CheckErr(err)
	stderr, err := cmd.StderrPipe()
	CheckErr(err)
	go io.Copy(os.Stdout, stdout)
	go io.Copy(os.Stderr, stderr)
	err = cmd.Run()
	CheckErr(err)
}

func Assert(value bool) {
	if !value {
		panic("assertion failed")
	}
}

func CheckErr(err error) {
	if err != nil {
		panic(err.Error())
	}
}

type CacheResult struct {
	FileExists       bool
	CacheEntryExists bool
	CacheIsValid     bool
}

func CheckUpdateCache(filePath string, extras ...string) CacheResult {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)
	result := checkCache(filePath, cachePath)

	currentTime := time.Now().Local()
	err := os.Chtimes(cachePath, currentTime, currentTime)
	CheckErr(err)

	return result
}

func checkCache(filePath, cachePath string) CacheResult {
	result := CacheResult{
		FileExists:       true,
		CacheEntryExists: true,
	}

	pathStat, err := os.Stat(filePath)
	if os.IsNotExist(err) {
		result.FileExists = false
	} else {
		CheckErr(err)
	}

	cacheFileStat, err := os.Stat(cachePath)
	if os.IsNotExist(err) {
		err := os.MkdirAll(filepath.Dir(cachePath), fs.ModeDir|fs.ModePerm)
		CheckErr(err)
		file, err := os.OpenFile(cachePath, os.O_RDWR|os.O_CREATE, fs.ModePerm)
		CheckErr(err)
		file.Close()

		result.CacheEntryExists = false
	} else {
		CheckErr(err)
	}

	if !result.CacheEntryExists || !result.FileExists {
		result.CacheIsValid = false
		return result
	}

	cacheModTime, pathModTime := cacheFileStat.ModTime(), pathStat.ModTime()
	result.CacheIsValid = pathModTime.Before(cacheModTime)
	return result
}
