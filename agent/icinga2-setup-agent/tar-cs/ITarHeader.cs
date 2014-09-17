using System;

namespace tar_cs
{
    public enum EntryType : byte
    {
        File = 0,
        FileObsolete = 0x30,
        HardLink = 0x31,
        SymLink = 0x32,
        CharDevice = 0x33,
        BlockDevice = 0x34,
        Directory = 0x35,
        Fifo = 0x36,
    }

    public interface ITarHeader
    {
        string FileName { get; set; }
        int Mode { get; set; }
        int UserId { get; set; }
        string UserName { get; set; }
        int GroupId { get; set; }
        string GroupName { get; set; }
        long SizeInBytes { get; set; }
        DateTime LastModification { get; set; }
        int HeaderSize { get; }
        EntryType EntryType { get; set; }
    }
}