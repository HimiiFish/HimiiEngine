/*******************************************************************************
 * @author: Yumihoshi
 * @email: xiaoyuesun915@gmail.com
 * @creationDate: 2025/6/22 下午5:49
 * @version: 1.0
 * @description:
 *******************************************************************************/
#ifndef HIMIIENGINEPROJECT_SINGLETON_H
#define HIMIIENGINEPROJECT_SINGLETON_H
/**
 * @file Singleton.h
 * @brief 控制台游戏专用线程安全单例模板基类
 *
 * 设计要点：
 * 1. 基于C++11 Magic Static的线程安全初始化
 * 2. 禁止拷贝构造和赋值操作
 * 3. 自动资源生命周期管理
 * 4. 支持继承的友好设计（需配合友元声明）
 *
 * 使用示例：
 * class GameManager : public Singleton<GameManager> {
 *     friend class Singleton<GameManager>;
 * private:
 *     GameManager() {  初始化  } // 必须私有构造函数
 * public:
 *     void update() { ... }
 * };
 *
 * // 使用单例：
 * GameManager::GetInstance().update();
 */
#include <iostream>
#include <mutex>
#include <memory>

template <typename T>
class Singleton {
public:
    // 获取单例实例
    static T& GetInstance() {
        static T instance;
        return instance;
    }

    // 禁用拷贝和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    // 保护构造函数/析构函数
    Singleton() = default;
    virtual ~Singleton() = default;
};

#endif // HIMIIENGINEPROJECT_SINGLETON_H
