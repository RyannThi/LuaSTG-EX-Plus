using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Eyes2D;

namespace CSharpTest
{
    enum MyEnum : byte
    {
        TESTA,
        TESTB,
        TESTC,
    }

    class Program
    {
        static void Main(string[] args)
        {
            ThreadSafeEnum<MyEnum> test = new ThreadSafeEnum<MyEnum>(MyEnum.TESTA);
            Console.WriteLine("{0}",test.Get());
            test.Set(MyEnum.TESTB);
            Console.WriteLine("{0}", test.Get());
            Console.ReadLine();
        }
    }
}
