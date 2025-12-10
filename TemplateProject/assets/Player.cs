using System;
using Himii;

namespace Game
{
    public class Player : Entity
    {
        void OnCreate()
        {
            Console.WriteLine("Player created!");
        }

        void OnUpdate(float ts)
        {
            // 在这里添加你的更新逻辑
        }
    }
}
