package com.lge.cmuteam3.client;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.*;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.awt.image.BufferedImage;
import java.util.Queue;
import javax.imageio.ImageIO;

public class Receiver extends Thread {
	private static final Logger LOG = LoggerFactory.getLogger(Receiver.class);

	String address;
	int port;
	int bufferSize;
	Queue<BufferedImage> queue = new LinkedList<>();
	boolean imageReady = false;
	Socket tcpSocket;
	
	private transient boolean running = false;

	private OnConnectListener onConnectlistener;

	public Receiver(String address, int port){
		this.address = address;
		this.port = port;
		String strBufferSize = FileProperties.get("client.bufferSize");
		this.bufferSize = Integer.parseInt(strBufferSize);
	}

	private void receiveImages() {
		try {
			LOG.info("Connect socket");
			tcpSocket = new Socket(address, port);
			InputStream inputStream = tcpSocket.getInputStream();
			long init = System.currentTimeMillis();

			onConnectlistener.onConnected();
			
			running = true;
			
			/* Receiving loop */
			int count = 0;
			while (running) {
				LOG.info("Connect socket3");
				byte[] sizeAr = new byte[4];
				readNBytes(inputStream, sizeAr, 0, 4);
				int size = ByteBuffer.wrap(sizeAr).asIntBuffer().get();
				byte[] imageAr = new byte[size];
				readNBytes(inputStream, imageAr, 0, size);
				queue.add(ImageIO.read(new ByteArrayInputStream(imageAr)));
				long current = System.currentTimeMillis();
				// System.out.println("gaenoo here : " + (current - init));
				init = current;

				count++;
				if (count >= bufferSize) {
					imageReady = true;
				}
			}
		} catch (ConnectException ce) {
			LOG.error(ce.getMessage());
			onConnectlistener.onFailed();
		} catch (IOException e) {
			e.printStackTrace();
			onConnectlistener.onFailed();
		} finally {
			try {
				if (tcpSocket != null) {
					tcpSocket.close();
				}
				running = false;
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	@Override
	public void run() {
		LOG.info("Receiver run");
		receiveImages();
	}

	public BufferedImage getImageFrame(){
		if (imageReady) {
			return queue.poll();
		}
		return null;
	}

	public int getRemainBufferSize() {
		return queue.size();
	}

	// Utils Refactoring
	private int readNBytes(InputStream inputStream, byte[] b, int off, int len) throws IOException {
        int n = 0;
        while (n < len) {
            int count = inputStream.read(b, off + n, len - n);
            if (count < 0)
                break;
            n += count;
        }
        return n;
    }
	
	public void stopSelf() {
		this.running = false;
	}

	public void setOnConnectListener(OnConnectListener onConnectlistener) {
		this.onConnectlistener = onConnectlistener;
	}
}