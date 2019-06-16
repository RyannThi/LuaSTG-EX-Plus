//code by Xiliusha
//线程安全枚举包装，用于保证枚举量在更改的时候是安全的
//基于4.6框架编写

using System.Threading;//线程库

namespace Eyes2D {
    /// <summary>
    /// 线程安全枚举包装，泛型T必须为枚举类型，其他类型不保证能正常使用
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class ThreadSafeEnum<T>
    {
        private T enum_object;
        private ReaderWriterLockSlim rwlock_object;

        /// <summary>
        /// 必须以一个值初始化
        /// </summary>
        /// <param name="init"></param>
        public ThreadSafeEnum(T init)
        {
            rwlock_object = new ReaderWriterLockSlim();
            Set(init);
        }

        /// <summary>
        /// 线程安全地设置枚举值
        /// </summary>
        /// <param name="v"></param>
        public void Set(T v)
        {
            rwlock_object.EnterWriteLock();
            enum_object = v;
            rwlock_object.ExitWriteLock();
        }
        /// <summary>
        /// 线程安全地获取枚举值
        /// </summary>
        /// <returns></returns>
        public T Get()
        {
            rwlock_object.EnterReadLock();
            T cache = enum_object;
            rwlock_object.ExitReadLock();
            return cache;
        }
    }
}
