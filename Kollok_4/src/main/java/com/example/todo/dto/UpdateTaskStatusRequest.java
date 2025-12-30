package com.example.todo.dto;

import jakarta.validation.constraints.Pattern;

public class UpdateTaskStatusRequest {

    @Pattern(regexp = "todo|in_progress|done")
    private String status;

    public String getStatus() { return status; }
    public void setStatus(String status) { this.status = status; }
}
