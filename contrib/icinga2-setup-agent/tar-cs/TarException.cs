using System;

namespace tar_cs
{
    public class TarException : Exception
    {
        public TarException(string message) : base(message)
        {
        }
    }
}