using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace ParadataViewer
{
    public interface IPluggableOwner
    {
        Task ExecuteQuery();

        QueryOptions QueryOptions { get; }

        Task<Int64> ExecuteSingleQueryAsync(string sql);

        Task<List<object[]>> ExecuteQueryAsync(string sql);

        string GetConvertedTimestampColumnName(string columnName);
    }
}
