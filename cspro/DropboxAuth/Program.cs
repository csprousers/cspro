using System;
using System.Linq;
using System.Threading.Tasks;

namespace DropboxAuth
{
    class Program
    {
        static async Task<int> Main(string[] args)
        {
            string locale = args.FirstOrDefault();
            DropboxAuthorizer authorizer = new DropboxAuthorizer();
            var access_token = await authorizer.DoAuthorizeAsync(locale);
            if (access_token != null)
            {
                Console.Write(access_token);
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }
}
