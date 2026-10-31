// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "icinga/host.hpp"
#include "perfdata/perfdatawriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/utils.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;

namespace
{

class PerfdataWriterFileFixture : public TestLoggerFixture
{
public:
	PerfdataWriterFileFixture() : m_Writer(new PerfdataWriter)
	{
		namespace fs = boost::filesystem;

		auto createObjects = [&]() {
			String config = R"CONFIG(
object CheckCommand "dummy" {
	command = "/bin/echo"
}
object Host "h1" {
	address = "h1"
	check_command = "dummy"
	enable_active_checks = false
	enable_passive_checks = true
}
)CONFIG";

			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
			expr->Evaluate(*ScriptFrame::GetCurrentFrame());
		};

		ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", createObjects));

		m_Host = Host::GetByName("h1");
		BOOST_REQUIRE(m_Host);

		m_Dir = fs::temp_directory_path() / fs::unique_path("icinga2-perfdatawriter-%%%%-%%%%");
		fs::create_directories(m_Dir / "perfdata");
		fs::create_directories(m_Dir / "tmp");

		m_Writer->SetName(m_Writer->GetReflectionType()->GetName());
		m_Writer->SetHostPerfdataPath((m_Dir / "perfdata" / "host-perfdata").string());
		m_Writer->SetHostTempPath((m_Dir / "tmp" / "host-perfdata").string());
		m_Writer->SetServicePerfdataPath((m_Dir / "perfdata" / "service-perfdata").string());
		m_Writer->SetServiceTempPath((m_Dir / "tmp" / "service-perfdata").string());
		// Each line is just the check result's output, so that the tests can identify check results.
		m_Writer->SetHostFormatTemplate("$host.output$");
		m_Writer->Register();
	}

	~PerfdataWriterFileFixture()
	{
		boost::system::error_code ec;
		boost::filesystem::remove_all(m_Dir, ec);
	}

	void ResumeWriter()
	{
		static_cast<ConfigObject::Ptr>(m_Writer)->OnConfigLoaded();
		m_Writer->SetActive(true);
		m_Writer->Activate();
		BOOST_REQUIRE(!m_Writer->IsPaused());
	}

	void PauseWriter() { static_cast<ConfigObject::Ptr>(m_Writer)->Pause(); }

	/**
	 * Sends check results whose output is "<index>:<padding>" to the writer.
	 *
	 * @param num Number of check results
	 * @param padding Number of padding bytes to append to each check result's output
	 */
	void ReceiveNumberedCheckResults(std::size_t num, std::size_t padding = 0)
	{
		std::size_t i = 0;
		::ReceiveCheckResults(m_Host, num, ServiceOK, [&](const CheckResult::Ptr& cr) {
			cr->SetOutput(GetRandomString(std::to_string(i++) + ":", padding));
		});
	}

	/**
	 * Splits the lines written by the writer and checks that the check results arrived complete and in order.
	 */
	static void RequireNumberedLines(const std::string& data, std::size_t num)
	{
		std::vector<std::string> lines;
		boost::split(lines, data, boost::is_any_of("\n"));
		BOOST_REQUIRE(!lines.empty() && lines.back().empty());
		lines.pop_back();

		BOOST_REQUIRE_EQUAL(lines.size(), num);
		for (std::size_t i = 0; i < num; ++i) {
			BOOST_REQUIRE_MESSAGE(
				lines[i].compare(0, std::to_string(i).length() + 1, std::to_string(i) + ":") == 0,
				"line " << i << " is out of order or garbled: " << lines[i].substr(0, 32)
			);
		}
	}

	/**
	 * Concatenates all rotated host perfdata files in the order they were rotated.
	 */
	std::string ReadRotatedHostPerfdata() const
	{
		std::vector<boost::filesystem::path> files;
		for (const auto& entry : boost::filesystem::directory_iterator{m_Dir / "perfdata"}) {
			files.emplace_back(entry.path());
		}
		std::sort(files.begin(), files.end());

		std::string data;
		for (const auto& file : files) {
			std::ifstream in{file.string(), std::ios::binary};
			data.append(std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{});
		}
		return data;
	}

	boost::filesystem::path m_Dir;
	Host::Ptr m_Host;
	PerfdataWriter::Ptr m_Writer;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(perfdata_perfdatawriter, PerfdataWriterFileFixture,
	*boost::unit_test::label("perfdata") )

BOOST_AUTO_TEST_CASE(flush_on_pause)
{
	ResumeWriter();

	ReceiveNumberedCheckResults(500);

	// Pause() must not return before everything that was queued has been written and the file was rotated.
	PauseWriter();

	RequireNumberedLines(ReadRotatedHostPerfdata(), 500);
}

#ifndef _WIN32
BOOST_AUTO_TEST_CASE(slow_disk_does_not_block_check_results)
{
	auto slowDir = m_Dir / "slow";
	auto fifo    = (slowDir / "host-perfdata").string();
	m_Writer->SetHostTempPath(fifo);

	// The writer can't open its file as long as the directory doesn't exist. A stream that failed to open isn't
	// "good", so the next rotation won't treat the (then existing) file as a leftover to rename but opens it.
	ResumeWriter();
	auto configObject = static_cast<ConfigObject::Ptr>(m_Writer);
	configObject->SetAuthority(false);

	// Simulate a disk that doesn't accept data: A FIFO that isn't read until told so.
	// Once its buffer (64 KiB on Linux) is full, every write() to it blocks.
	boost::filesystem::create_directories(slowDir);
	BOOST_REQUIRE_EQUAL(mkfifo(fifo.c_str(), 0600), 0);
	int readFd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK);
	BOOST_REQUIRE_GE(readFd, 0);

	std::mutex mutex;
	std::condition_variable cv;
	bool checkResultsSent = false;
	std::atomic<bool> writerPaused{false};
	std::string received;

	// The "disk" starts working again as soon as all check results were sent, but at the latest after 2s.
	std::thread disk([&]() {
		{
			std::unique_lock lock(mutex);
			cv.wait_for(lock, 2s, [&]() { return checkResultsSent; });
		}

		char buf[4096];
		for (;;) {
			auto n = read(readFd, buf, sizeof(buf));
			if (n > 0) {
				received.append(buf, n);
			} else if (writerPaused) {
				break;
			} else {
				std::this_thread::sleep_for(1ms);
			}
		}
	});

	configObject->SetAuthority(true);

	// 300 * 1 KiB is much more than the FIFO and the file stream can buffer.
	auto start = std::chrono::steady_clock::now();
	ReceiveNumberedCheckResults(300, 1024);
	auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

	{
		std::unique_lock lock(mutex);
		checkResultsSent = true;
	}
	cv.notify_all();

	// The check results are written by the writer's work queue, so the caller must not wait for the disk.
	BOOST_CHECK_MESSAGE(
		elapsedMs < 1000, "Receiving the check results took " << elapsedMs << "ms, i.e. they waited for the disk."
	);

	// Pausing waits for the queue, so after it everything must have reached the disk.
	configObject->SetAuthority(false);
	writerPaused = true;
	disk.join();
	close(readFd);

	RequireNumberedLines(received, 300);
}
#endif /* _WIN32 */

BOOST_AUTO_TEST_SUITE_END()
