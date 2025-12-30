package com.example.todo.controller;

import com.example.todo.dto.CreateTaskRequest;
import com.example.todo.dto.UpdateTaskRequest;
import com.example.todo.dto.UpdateTaskStatusRequest;
import com.example.todo.model.Task;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

@RestController
@RequestMapping("/tasks")
public class TaskController {

    private final Map<Long, Task> tasks = new ConcurrentHashMap<>();
    private final AtomicLong idGenerator = new AtomicLong(1);

    @GetMapping
    public List<Task> getAllTasks() {
        return new ArrayList<>(tasks.values());
    }

    @GetMapping("/{id}")
    public ResponseEntity<Task> getTaskById(@PathVariable Long id) {
        Task task = tasks.get(id);
        if (task == null) return ResponseEntity.status(HttpStatus.NOT_FOUND).build();
        return ResponseEntity.ok(task);
    }

    @PostMapping
    public ResponseEntity<Task> createTask(@Valid @RequestBody CreateTaskRequest request) {
        Long id = idGenerator.getAndIncrement();
        String status = request.getStatus() == null ? "todo" : request.getStatus();

        Task task = new Task(id, request.getTitle(), request.getDescription(), status);
        tasks.put(id, task);

        return ResponseEntity.status(HttpStatus.CREATED).body(task);
    }

    @PutMapping("/{id}")
    public ResponseEntity<Task> updateTask(
            @PathVariable Long id,
            @Valid @RequestBody UpdateTaskRequest request
    ) {
        Task existing = tasks.get(id);
        if (existing == null) return ResponseEntity.status(HttpStatus.NOT_FOUND).build();

        existing.setTitle(request.getTitle());
        existing.setDescription(request.getDescription());
        existing.setStatus(request.getStatus());

        return ResponseEntity.ok(existing);
    }

    @PatchMapping("/{id}")
    public ResponseEntity<Task> patchTask(
            @PathVariable Long id,
            @Valid @RequestBody UpdateTaskStatusRequest request
    ) {
        Task existing = tasks.get(id);
        if (existing == null) return ResponseEntity.status(HttpStatus.NOT_FOUND).build();

        if (request.getStatus() != null) {
            existing.setStatus(request.getStatus());
        }

        return ResponseEntity.ok(existing);
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> deleteTask(@PathVariable Long id) {
        Task removed = tasks.remove(id);
        if (removed == null) return ResponseEntity.status(HttpStatus.NOT_FOUND).build();
        return ResponseEntity.noContent().build();
    }
}
