#pragma once

// 클라이언트에서 시간을 측정하는 클래스
// 전체 실행시간, 일시정지, 프레임간 시간(elapsed time)등 시간과 관련한 기능을 제공
class Timer
{
public:
	Timer();

	float TotalTime() const;	// 전체 실행시간
	float PlayTime() const;		// 일시정지된 시간을 제외한 실행시간
	float ElapsedTime() const;	// 이전 프레임 시각에서 현재 프레임 시각을 뺀 사이 시간

	int Fps();

	void Reset();		// 타이머 초기화
	void Start();		// 타이머 시작
	void Stop();		// 타이머 일시정지
	void Tick();		// elapsed_time 측정

private:
	double seconds_per_count_ = 0;
	double elapsed_time_ = -1;

	static constexpr int kFpsBufferSize{ 60 };
	std::array<double, kFpsBufferSize> fps_buffer_{};
	UINT fps_buffer_index_{ 0 };

	__int64 base_time_ = 0;		// 실행시각
	__int64 paused_time_ = 0;	// 정지된 시간
	__int64 stop_time_ = 0;		// 정지를 시작한 시각
	__int64 prev_time_ = 0;		// 이전 프레임 시각
	__int64 curr_time_ = 0;		// 현재 시각

	bool is_stop_ = false;
};

