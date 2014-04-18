using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace tar_cs
{
    /// <summary>
    /// Extract contents of a tar file represented by a stream for the TarReader constructor
    /// </summary>
    public class TarReader
    {
        private readonly byte[] dataBuffer = new byte[512];
        private readonly UsTarHeader header;
        private readonly Stream inStream;
        private long remainingBytesInFile;

        /// <summary>
        /// Constructs TarReader object to read data from `tarredData` stream
        /// </summary>
        /// <param name="tarredData">A stream to read tar archive from</param>
        public TarReader(Stream tarredData)
        {
            inStream = tarredData;
            header = new UsTarHeader();
        }

        public ITarHeader FileInfo
        {
            get { return header; }
        }

        /// <summary>
        /// Read all files from an archive to a directory. It creates some child directories to
        /// reproduce a file structure from the archive.
        /// </summary>
        /// <param name="destDirectory">The out directory.</param>
        /// 
        /// CAUTION! This method is not safe. It's not tar-bomb proof. 
        /// {see http://en.wikipedia.org/wiki/Tar_(file_format) }
        /// If you are not sure about the source of an archive you extracting,
        /// then use MoveNext and Read and handle paths like ".." and "../.." according
        /// to your business logic.
        public void ReadToEnd(string destDirectory)
        {
            while (MoveNext(false))
            {
                string fileNameFromArchive = FileInfo.FileName;
                string totalPath = destDirectory + Path.DirectorySeparatorChar + fileNameFromArchive;
                if(UsTarHeader.IsPathSeparator(fileNameFromArchive[fileNameFromArchive.Length -1]) || FileInfo.EntryType == EntryType.Directory)
                {
                    // Record is a directory
                    Directory.CreateDirectory(totalPath);
                    continue;
                }
                // If record is a file
                string fileName = Path.GetFileName(totalPath);
                string directory = totalPath.Remove(totalPath.Length - fileName.Length);
                Directory.CreateDirectory(directory);
                using (FileStream file = File.Create(totalPath))
                {
                    Read(file);
                }
            }
        }
        
        /// <summary>
        /// Read data from a current file to a Stream.
        /// </summary>
        /// <param name="dataDestanation">A stream to read data to</param>
        /// 
        /// <seealso cref="MoveNext"/>
        public void Read(Stream dataDestanation)
        {
            Debug.WriteLine("tar stream position Read in: " + inStream.Position);
            int readBytes;
            byte[] read;
            while ((readBytes = Read(out read)) != -1)
            {
                Debug.WriteLine("tar stream position Read while(...) : " + inStream.Position);
                dataDestanation.Write(read, 0, readBytes);
            }
            Debug.WriteLine("tar stream position Read out: " + inStream.Position);
        }

        protected int Read(out byte[] buffer)
        {
            if(remainingBytesInFile == 0)
            {
                buffer = null;
                return -1;
            }
            int align512 = -1;
            long toRead = remainingBytesInFile - 512;

            if (toRead > 0) 
                toRead = 512;
            else
            {
                align512 = 512 - (int)remainingBytesInFile;
                toRead = remainingBytesInFile;
            }

            int bytesRead = 0;
            long bytesRemainingToRead = toRead;
            do
            {

                bytesRead = inStream.Read(dataBuffer, (int)(toRead-bytesRemainingToRead), (int)bytesRemainingToRead);
                bytesRemainingToRead -= bytesRead;
                remainingBytesInFile -= bytesRead;
            } while (bytesRead < toRead && bytesRemainingToRead > 0);
            
            if(inStream.CanSeek && align512 > 0)
            {
                inStream.Seek(align512, SeekOrigin.Current);
            }
            else
                while(align512 > 0)
                {
                    inStream.ReadByte();
                    --align512;
                }
                
            buffer = dataBuffer;
            return bytesRead;
        }

        /// <summary>
        /// Check if all bytes in buffer are zeroes
        /// </summary>
        /// <param name="buffer">buffer to check</param>
        /// <returns>true if all bytes are zeroes, otherwise false</returns>
        private static bool IsEmpty(IEnumerable<byte> buffer)
        {
            foreach(byte b in buffer)
            {
                if (b != 0) return false;
            }
            return true;
        }

        /// <summary>
        /// Move internal pointer to a next file in archive.
        /// </summary>
        /// <param name="skipData">Should be true if you want to read a header only, otherwise false</param>
        /// <returns>false on End Of File otherwise true</returns>
        /// 
        /// Example:
        /// while(MoveNext())
        /// { 
        ///     Read(dataDestStream); 
        /// }
        /// <seealso cref="Read(Stream)"/>
        public bool MoveNext(bool skipData)
        {
            Debug.WriteLine("tar stream position MoveNext in: " + inStream.Position);
            if (remainingBytesInFile > 0)
            {
                if (!skipData)
                {
                    throw new TarException(
                        "You are trying to change file while not all the data from the previous one was read. If you do want to skip files use skipData parameter set to true.");
                }
                // Skip to the end of file.
                if (inStream.CanSeek)
                {
                    long remainer = (remainingBytesInFile%512);
                    inStream.Seek(remainingBytesInFile + (512 - (remainer == 0 ? 512 : remainer) ), SeekOrigin.Current);
                }
                else
                {
                    byte[] buffer;
                    while (Read(out buffer) > 0)
                    {
                    }
                }
            }

            byte[] bytes = header.GetBytes();
            int headerRead;
            int bytesRemaining = header.HeaderSize;
            do
            {
                headerRead = inStream.Read(bytes, header.HeaderSize - bytesRemaining, bytesRemaining);
                bytesRemaining -= headerRead;
                if (headerRead <= 0 && bytesRemaining > 0)
                {
                    throw new TarException("Can not read header");
                }
            } while (bytesRemaining > 0);

            if(IsEmpty(bytes))
            {
                bytesRemaining = header.HeaderSize;
                do
                {
                    headerRead = inStream.Read(bytes, header.HeaderSize - bytesRemaining, bytesRemaining);
                    bytesRemaining -= headerRead;
                    if (headerRead <= 0 && bytesRemaining > 0)
                    {
                        throw new TarException("Broken archive");
                    }
                    
                } while (bytesRemaining > 0);
                if (bytesRemaining == 0 && IsEmpty(bytes))
                {
                    Debug.WriteLine("tar stream position MoveNext  out(false): " + inStream.Position);
                    return false;
                }
                throw new TarException("Broken archive");
            }

            if (header.UpdateHeaderFromBytes())
            {
                throw new TarException("Checksum check failed");
            }

            remainingBytesInFile = header.SizeInBytes;

            Debug.WriteLine("tar stream position MoveNext  out(true): " + inStream.Position);
            return true;
        }
    }
}