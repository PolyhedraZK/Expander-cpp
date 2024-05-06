#pragma once
// The interface design of the polynomial commitment scheme.

class PolynomialCommitment
{
public:
    // The setup phase of the polynomial commitment scheme.
    // In transparent setup, implment this function with empty body.
    virtual void setup() = 0;

    // The commit phase of the polynomial commitment scheme.
    virtual void commit() = 0;

    // The open phase of the polynomial commitment scheme.
    virtual void open() = 0;
};