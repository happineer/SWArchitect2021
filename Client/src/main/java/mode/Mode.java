package mode;

public interface Mode {
	enum RunningButtonMode {
		DISABLE_ALL, DISABLE_ALL_EXCEPT_CURRENT, ENABLE_ALL
	}

	String getModeName();
	void start();
	void stop();
	boolean needTransferSocket();
	RunningButtonMode getRunningButtonMode();
	boolean isRunning();
}
