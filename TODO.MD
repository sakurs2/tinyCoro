# Todo list

### 正在解决中

- [ ] benchmark结果统计
- [ ] perf火焰图性能优化
- [ ] 为cmake添加各项环境检查
- [ ] README里添加更新日志
- [ ] 添加注释
- [ ] 测试clang编译器
- [ ] 为benchtests添加threadpool版本

### 已完成

- [x] 添加perf火焰图生成脚本
- [x] 更改所有同步组件register和resume的方式
- [x] 添加负载均衡模块使得任务支持跨线程
- [x] 解决context stop过慢的问题
- [x] 配置文件重新设计
- [x] 更改context的stop为notify_stop，并修改测试用例
- [x] 测试数值改为配置项
- [x] 添加版本号信息
- [x] 为benchtests添加threadpool版本

### 未来待完成

- [ ] 添加更细致的注释
- [ ] 拓展新功能
- [ ] 重构优化
- [ ] 添加任务取消机制
- [ ] 解决项目内标记的TODO
- [ ] 为awaiter添加基类
- [ ] 动态链接库接口可见性
- [ ] 添加co_yield功能
- [ ] 为uring添加polling模式
- [ ] 修复高QPS下部分响应失败
- [ ] 修复内存invalid read的问题

## 后续实验

- [x] 功能测试
- [x] 内存安全测试
- [x] 性能测试
- [x] 测试明确、理智、集中
