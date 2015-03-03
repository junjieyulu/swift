// RUN: %target-parse-verify-swift

// REQUIRES: objc_interop

@objc class Redecl1 { // expected-note{{implicit deinitializer declared here}}
  @objc init() { } // expected-note{{initializer 'init()' declared here}}

  @objc
  func method1() { } // expected-note 2{{method 'method1()' declared here}}

  @objc var value: Int // expected-note{{setter for 'value' declared here}}

  @objc(wibble) var other: Int
  // expected-note@-1{{getter for 'other' declared here}}
  // expected-note@-2{{setter for 'other' declared here}}
}

extension Redecl1 {
  @objc(method1)
  func method1_alias() { } // expected-error{{method 'method1_alias()' with Objective-C selector 'method1' conflicts with method 'method1()'}}
}

extension Redecl1 {
  @objc var method1_var_alias: Int {
    @objc(method1) get { return 5 } // expected-error{{getter for 'method1_var_alias' with Objective-C selector 'method1' conflicts with method 'method1()'}}

    @objc(method2:) set { } // expected-note{{setter for 'method1_var_alias' declared here}}
  }

  @objc subscript (i: Int) -> Redecl1 {
    get { return self } // expected-note{{subscript getter declared here}}
    set { }
  }
}

extension Redecl1 {
  @objc
  func method2(x: Int) { } // expected-error{{method 'method2' with Objective-C selector 'method2:' conflicts with setter for 'method1_var_alias'}}

  @objc(objectAtIndexedSubscript:)
  func indexed(x: Int) { } // expected-error{{method 'indexed' with Objective-C selector 'objectAtIndexedSubscript:' conflicts with subscript getter}}

  @objc(init)
  func initialize() { } // expected-error{{method 'initialize()' with Objective-C selector 'init' conflicts with initializer 'init()'}}

  @objc
  func dealloc() { } // expected-error{{method 'dealloc()' with Objective-C selector 'dealloc' conflicts with implicit deinitializer}}

  @objc func setValue(x: Int) { } // expected-error{{method 'setValue' with Objective-C selector 'setValue:' conflicts with setter for 'value'}}
}

extension Redecl1 {
  @objc func setWibble(other: Int) { } // expected-error{{method 'setWibble' with Objective-C selector 'setWibble:' conflicts with setter for 'other'}}
  @objc func wibble() -> Int { return 0 } // expected-error{{method 'wibble()' with Objective-C selector 'wibble' conflicts with getter for 'other'}}
}
