//> Classes lox-class
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;
import.util.*

/* Classes lox-class < Classes lox-class-callable
class LoxClass {
*/
//> lox-class-callable
class LoxClass implements LoxCallable {
    //< lox-class-callable
    final String name;
    //> Inheritance lox-class-superclass-field
    final LoxClass superclass;
    //< Inheritance lox-class-superclass-field

    LoxClass(String name, LoxClass superclass, Map<String, LoxFunction> methods, LoxClass metaclass){
        super(metaclass);
        this.name = name;
        this.superclass = superclass;
        this.methods = methods;
    }
    /* Classes lox-class < Classes lox-class-methods


      LoxClass(String name) {
        this.name = name;
      }
    */
//> lox-class-methods
    private final Map<String, LoxFunction> methods;

    /* Classes lox-class-methods < Inheritance lox-class-constructor
      LoxClass(String name, Map<String, LoxFunction> methods) {
    */
//> Inheritance lox-class-constructor
    LoxClass(String name, LoxClass superclass,
             Map<String, LoxFunction> methods) {
        this.superclass = superclass;
//< Inheritance lox-class-constructor
        this.name = name;
        this.methods = methods;
    }
    //< lox-class-methods
//> lox-class-find-method
    LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

//> Inheritance find-method-recurse-superclass
        if (superclass != null) {
            return superclass.findMethod(name);
        }

//< Inheritance find-method-recurse-superclass
        return null;
    }
//< lox-class-find-method

    @Override
    public String toString() {
        return name;
    }
    //> lox-class-call-arity
    @Override
    public Object call(Interpreter interpreter,
                       List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
//> lox-class-call-initializer
        LoxFunction initializer = findMethod("init");
        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }

//< lox-class-call-initializer
        return instance;
    }

    @Override
    public int arity() {
/* Classes lox-class-call-arity < Classes lox-initializer-arity
    return 0;
*/
//> lox-initializer-arity
        LoxFunction initializer = findMethod("init");
        if (initializer == null) return 0;
        return initializer.arity();
//< lox-initializer-arity
    }

    LoxClass root(){
        LoxClass c = this;
        while (c.superclass != null) c = c.superclass;
        return c;
    }
    LoxFunction findMethodTopDown(String name) {
        LoxFunction found = null;
        for (LoxClass c = root(); c != null; c = c.subclassToward(this)) {
            if (c.methods.containsKey(name)) {
                return c.methods.get(name);
            }
            if (c == this) break;
        }
        return null;
    }

    List<LoxClass> chainRootToThis() {
        List<LoxClass> chain = new ArrayList<>();
        for (LoxClass c = this; c != null; c = c.superclass) chain.add(c);
        Collections.reverse(chain);
        return chain;
    }

    LoxFunction findMethodTopDown(String name) {
        for (LoxClass c : chainRootToThis()) {
            LoxFunction m = c.methods.get(name);
            if (m != null) return m;
        }
        return null;
    }
    LoxFunction findInnerMethod(String name, LoxClass definingClass, LoxClass runtimeClass) {
        List<LoxClass> chain = runtimeClass.chainRootToThis();
        int start = -1;
        for (int i = 0; i < chain.size(); i++) {
            if (chain.get(i) == definingClass) { start = i; break; }
        }
        if (start == -1) return null;

        for (int i = start + 1; i < chain.size(); i++) {
            LoxFunction m = chain.get(i).methods.get(name);
            if (m != null) return m;
        }
        return null;
    }
//< lox-class-call-arity
}
