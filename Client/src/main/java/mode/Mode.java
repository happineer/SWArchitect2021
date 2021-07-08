package mode;

public interface Mode {
	String getModeName();
	void start();
	void stop();
	boolean needTransferSocket();
}
