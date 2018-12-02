import java.net.*;
import java.io.*;

class SocketThread extends Thread{
	private Socket conn;
	private static int nFiles = 0;

	public SocketThread(Socket conn){
		this.conn = conn;
	}

	public void run(){
		InetAddress addr = conn.getInetAddress();
		int port = conn.getPort();
		System.out.println("connection from client: " + addr.toString() + ":" + port);
		InputStream is = null;
		BufferedReader br = null;
		OutputStream os = null;
		DataOutputStream dos = null;
		try{
			is = conn.getInputStream();
			br =  new BufferedReader(new InputStreamReader(is));
			os = conn.getOutputStream();
			dos = new DataOutputStream(os);
			int thisNfiles = 0;
			while (true){
				String filename = br.readLine();

				if (filename == null){
					System.err.println(addr.toString() + ":" + port + " closed connection");
					break;
				}
				filename = filename.trim();
				System.out.print("client " + addr.toString() + ":" + port
								+ " required file \'" + filename + "\'" + "\n");

				File file = null;
				FileInputStream fis = null;

				long filesize = 0;
				try{
					//String pwd = System.getProperty("user.dir");
					//System.err.println("current working directory: " + pwd);
					file = new File(filename);
					fis = new FileInputStream(file);
					filesize = file.length();
				} catch (IOException ex){
					ex.printStackTrace();
					System.err.println("cannot open \'" + filename + "\'");
					dos.writeLong(0);
					continue;
				}

				System.out.print("size of \'" + filename + "\': " + filesize + "\n");
				//send filesize
				dos.writeLong(filesize);
				System.err.print("sent filesize to " + addr.toString() + ":" + port + "\n");
				byte[] buff = new byte[4096];
				int nBytes = 0;
				boolean fileError = false;
				while (nBytes != -1){

					try{
						nBytes = fis.read(buff);
					} catch (IOException ex){
						ex.printStackTrace();
						fileError = true;
						fis.close();
					}

					if (nBytes < 0){
						break;
					}

					dos.write(buff, 0, nBytes);
					//System.err.println("sent " + nBytes + " bytes to " + addr.toString()
					//				+ ":" + port);
				}

				if (fileError)
					continue;

				System.out.println("sending \'" + filename + "\' to " + addr.toString() 
								+ ":" + port + " done");
				thisNfiles ++;
				System.out.println("sent " + thisNfiles + " files to " + addr.toString()
								+ ":" + port);

				increaseCount();
			}
		} catch (IOException ex){
			ex.printStackTrace();
		} finally {
			System.err.println("closing connection to " + addr.toString() + ":"
								+ port);
			try{
				dos.close();
				os.close();
				br.close();
				is.close();
				conn.close();
			} catch (IOException ex2){
				ex2.printStackTrace();
			}
			System.out.println("connection to " + addr.toString() + ":" + port + " closed");
		}
	}

	public synchronized void increaseCount(){
		nFiles ++;
		System.out.println("total files sent: " + nFiles);
	}
}

public class server {
	public static void main(String[] args){
		ServerSocket servSock = null;
		try{
			servSock = new ServerSocket(6969);
			servSock.setReuseAddress(true);
			Socket conn = null;
			while (true){
				try{
					conn = servSock.accept();
					SocketThread st = new SocketThread(conn);
					st.start();
				} catch (IOException ex){
					ex.printStackTrace();
					continue;
				}
			}
		} catch (IOException ex){
			ex.printStackTrace();
			System.exit(-1);
		}
	}
}
