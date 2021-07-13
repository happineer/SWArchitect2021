package com.lge.cmuteam3.client.network;

import com.jcraft.jsch.*;
import com.lge.cmuteam3.client.FileProperties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.*;
import java.util.Properties;
import java.util.Vector;

public class ScpProxy {
    private static final Logger LOG = LoggerFactory.getLogger(ScpProxy.class);

    private final String learningFolder;
    private final String keyFilePath;
    private final String password;
    private final String id;
    private final String serverIp;
    private final int port;
    private final String rescanScript;

    public ScpProxy() {
        FileProperties fileProperties = FileProperties.getInstance();
        keyFilePath = fileProperties.getProperty("client.ssh.keyFilePath");
        id = fileProperties.getProperty("server.ssh.id");
        password = fileProperties.getProperty("server.ssh.password");
        serverIp = fileProperties.getProperty("server.ip");
        port = Integer.parseInt(fileProperties.getProperty("server.ssh.port"));
        learningFolder = fileProperties.getProperty("server.ssh.learningFolder");
        rescanScript = fileProperties.getProperty("server.ssh.rescanScript");
    }

    public boolean createFolder(String name) {
        String targetDir = learningFolder + name;

        try {
            Session session = createSession(id, serverIp, port, keyFilePath, null, password);

            String command = "mkdir -p " + targetDir;
            LOG.info("ssh command:" + command);
            sendCommand(session, command);
        } catch (Exception e) {
            e.printStackTrace();
            LOG.info(e.getMessage());
            return false;
        }
        return true;
    }

    public boolean rescan() {
        try {
            Session session = createSession(id, serverIp, port, keyFilePath, null, password);

            LOG.info("ssh command:" + rescanScript);
            sendCommand(session, rescanScript);
        } catch (Exception e) {
            e.printStackTrace();
            LOG.info(e.getMessage());
            return false;
        }
        return true;
    }


    public boolean sendFile(File file, String targetFolder) {
        try {
            Session session = createSession(id, serverIp, port, keyFilePath, null, password);
            String targetDir = learningFolder + targetFolder;

            LOG.info("path :" + file.getAbsolutePath());
            LOG.info("name :" + file.getName());

            copyLocalToRemote(session, file.getAbsolutePath(), targetDir, file.getName());
        } catch (JSchException | IOException e) {
            e.printStackTrace();
            LOG.error("sendFile error:" + e.getMessage());
            return false;
        }

        return true;
    }

    public boolean receiveFolder(String sourceFolder, String targetFolder) {
        Channel channel = null;
        ChannelSftp channelSftp = null;

        try {
            Session session = createSession(id, serverIp, port, keyFilePath, null, password);

            LOG.info("sourceFolder :" + sourceFolder);
            LOG.info("targetFolder :" + targetFolder);

            channel = session.openChannel("sftp");
            channel.connect();

            channelSftp = (ChannelSftp) channel;

            LOG.info("PWD:" + channelSftp.pwd());
            channelSftp.cd(sourceFolder);
            Vector ls = channelSftp.ls("*");

            int size = ls.size();
            LOG.info("Unknown images size :" + size);

            for (Object l : ls) {
                ChannelSftp.LsEntry entry = (ChannelSftp.LsEntry) l;
                copyRemoteToLocal(session, sourceFolder, targetFolder, entry.getFilename());
            }
        } catch (JSchException | SftpException | IOException e) {
            e.printStackTrace();
            LOG.error("sendFile error:" + e.getMessage());
            return false;
        }

        return true;
    }

    private Session createSession(String user, String host, int port, String keyFilePath, String keyPassword, String password) throws JSchException {
        JSch jsch = new JSch();

        if (keyFilePath != null) {
            if (keyPassword != null) {
                jsch.addIdentity(keyFilePath, keyPassword);
            } else {
                jsch.addIdentity(keyFilePath);
            }
        }

        Properties config = new java.util.Properties();
        config.put("StrictHostKeyChecking", "no");
        Session session = jsch.getSession(user, host, port);
        session.setConfig(config);
        session.setPassword(password);

        session.connect();

        return session;
    }

    public void sendCommand(Session session, String command) {
        ChannelExec channel = null;

        try {
            channel = (ChannelExec) session.openChannel("exec");
            channel.setCommand(command);
            ByteArrayOutputStream responseStream = new ByteArrayOutputStream();
            channel.setOutputStream(responseStream);
            channel.connect();

            while (channel.isConnected()) {
                Thread.sleep(100);
            }

            String responseString = responseStream.toString();
            LOG.info("ssh response:" + responseString);
        } catch (JSchException e) {
            LOG.warn("ssh exception:" + e.getMessage());
        } catch (InterruptedException e) {
            LOG.info("wait interrupted");
        } finally {
            if (channel != null) {
                channel.disconnect();
            }
        }
    }

    private void copyLocalToRemote(Session session, String from, String to, String fileName) throws JSchException, IOException {
        boolean ptimestamp = true;

        // exec 'scp -t rfile' remotely
        String command = "scp " + (ptimestamp ? "-p" : "") + " -t " + to;
        Channel channel = session.openChannel("exec");
        ((ChannelExec) channel).setCommand(command);

        LOG.info("scp:command:" + command);

        // get I/O streams for remote scp
        OutputStream out = channel.getOutputStream();
        InputStream in = channel.getInputStream();

        channel.connect();

        if (checkAck(in) != 0) {
            System.exit(0);
        }

        File _lfile = new File(from);

        if (ptimestamp) {
            command = "T" + (_lfile.lastModified() / 1000) + " 0";
            // The access time should be sent here,
            // but it is not accessible with JavaAPI ;-<
            command += (" " + (_lfile.lastModified() / 1000) + " 0\n");
            out.write(command.getBytes());
            out.flush();
            if (checkAck(in) != 0) {
                System.exit(0);
            }
        }

        // send "C0644 fileSize filename", where filename should not include '/'
        long fileSize = _lfile.length();
        command = "C0644 " + fileSize + " ";
        LOG.info("from:" + from);

        command += fileName;

        LOG.info("command:" + command);

        command += "\n";
        out.write(command.getBytes());
        out.flush();

        if (checkAck(in) != 0) {
            System.exit(0);
        }

        // send a content of lfile
        FileInputStream fis = new FileInputStream(from);
        byte[] buf = new byte[1024];
        while (true) {
            int len = fis.read(buf, 0, buf.length);
            if (len <= 0) break;
            out.write(buf, 0, len); //out.flush();
        }

        // send '\0'
        buf[0] = 0;
        out.write(buf, 0, 1);
        out.flush();

        if (checkAck(in) != 0) {
            System.exit(0);
        }
        out.close();
        fis.close();

        channel.disconnect();
        session.disconnect();
    }

    public int checkAck(InputStream in) throws IOException {
        int b = in.read();
        // b may be 0 for success,
        //          1 for error,
        //          2 for fatal error,
        //         -1
        if (b == 0) return b;
        if (b == -1) return b;

        if (b == 1 || b == 2) {
            StringBuffer sb = new StringBuffer();
            int c;
            do {
                c = in.read();
                sb.append((char) c);
            }
            while (c != '\n');
            if (b == 1) { // error
                System.out.print(sb.toString());
            }
            if (b == 2) { // fatal error
                System.out.print(sb.toString());
            }
        }
        return b;
    }

    private void copyRemoteToLocal(Session session, String fromServer, String toLocal, String fileName) throws JSchException, IOException {
        fromServer = fromServer + File.separator + fileName;
        String prefix = null;

        if (new File(toLocal).isDirectory()) {
            prefix = toLocal + File.separator;
        }

        String command = "scp -f " + fromServer;
        Channel channel = session.openChannel("exec");
        ((ChannelExec) channel).setCommand(command);

        // get I/O streams for remote scp
        OutputStream out = channel.getOutputStream();
        InputStream in = channel.getInputStream();

        channel.connect();

        byte[] buf = new byte[1024];

        // send '\0'
        buf[0] = 0;
        out.write(buf, 0, 1);
        out.flush();

        while (true) {
            int c = checkAck(in);
            if (c != 'C') {
                break;
            }

            // read '0644 '
            in.read(buf, 0, 5);

            long filesize = 0L;
            while (true) {
                if (in.read(buf, 0, 1) < 0) {
                    // error
                    break;
                }
                if (buf[0] == ' ') break;
                filesize = filesize * 10L + (long) (buf[0] - '0');
            }

            String file = null;
            for (int i = 0; ; i++) {
                in.read(buf, i, 1);
                if (buf[i] == (byte) 0x0a) {
                    file = new String(buf, 0, i);
                    break;
                }
            }

            System.out.println("file-size=" + filesize + ", file=" + file);

            // send '\0'
            buf[0] = 0;
            out.write(buf, 0, 1);
            out.flush();

            // read a content of lfile
            FileOutputStream fos = new FileOutputStream(prefix == null ? toLocal : prefix + file);
            int foo;
            while (true) {
                if (buf.length < filesize) foo = buf.length;
                else foo = (int) filesize;
                foo = in.read(buf, 0, foo);
                if (foo < 0) {
                    // error
                    break;
                }
                fos.write(buf, 0, foo);
                filesize -= foo;
                if (filesize == 0L) break;
            }

            if (checkAck(in) != 0) {
                System.exit(0);
            }

            // send '\0'
            buf[0] = 0;
            out.write(buf, 0, 1);
            out.flush();

            try {
                if (fos != null) fos.close();
            } catch (Exception ex) {
                System.out.println(ex);
            }
        }

        channel.disconnect();
    }
}
