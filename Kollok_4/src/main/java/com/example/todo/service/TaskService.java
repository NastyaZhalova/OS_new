package com.example.todo.service;

import com.example.todo.dto.CreateTaskRequest;
import com.example.todo.dto.UpdateTaskRequest;
import com.example.todo.dto.UpdateTaskStatusRequest;
import com.example.todo.model.Task;
import com.example.todo.repository.TaskRepository;
import org.springframework.cache.annotation.CacheEvict;
import org.springframework.cache.annotation.Cacheable;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Optional;

@Service
public class TaskService {

    private final TaskRepository repository;

    public TaskService(TaskRepository repository) {
        this.repository = repository;
    }

    @Cacheable("tasks")
    public List<Task> getAllTasks() {
        return repository.findAll();
    }

    public Optional<Task> getTaskById(Long id) {
        return repository.findById(id);
    }

    @CacheEvict(value = "tasks", allEntries = true)
    public Task createTask(CreateTaskRequest request) {
        String status = request.getStatus() == null ? "todo" : request.getStatus();
        Task task = new Task(null, request.getTitle(), request.getDescription(), status);
        return repository.save(task);
    }

    @CacheEvict(value = "tasks", allEntries = true)
    public Optional<Task> updateTask(Long id, UpdateTaskRequest request) {
        return repository.findById(id).map(existing -> {
            existing.setTitle(request.getTitle());
            existing.setDescription(request.getDescription());
            existing.setStatus(request.getStatus());
            return repository.save(existing);
        });
    }

    @CacheEvict(value = "tasks", allEntries = true)
    public Optional<Task> patchTask(Long id, UpdateTaskStatusRequest request) {
        return repository.findById(id).map(existing -> {
            if (request.getStatus() != null) {
                existing.setStatus(request.getStatus());
            }
            return repository.save(existing);
        });
    }

    @CacheEvict(value = "tasks", allEntries = true)
    public boolean deleteTask(Long id) {
        if (!repository.existsById(id)) {
            return false;
        }
        repository.deleteById(id);
        return true;
    }
}
