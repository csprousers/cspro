namespace Stygitan
{
    internal static class Program
    {
        static void Main()
        {
            ApplicationConfiguration.Initialize();
            Application.Run(new MainForm());
        }
    }
}